#include "AnimationController.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "GameObject.h"
#include <algorithm>

namespace {
	/// <summary>
	/// 指定した時間に最も近いキーフレームの行列を取得します。
	/// </summary>
	DirectX::XMMATRIX SampleGlobalMatrixNearest(const AnimationChannel& ch, float tickTime)
	{
		int best = 0;
		float bestDiff = FLT_MAX;

		for (int i = 0; i < (int)ch.matrixKeys.size(); ++i)
		{
			float d = fabsf(ch.matrixKeys[i].time - tickTime);
			if (d < bestDiff)
			{
				bestDiff = d;
				best = i;
			}
		}

		return DirectX::XMLoadFloat4x4(&ch.matrixKeys[best].matrix);
	}
}

//-----------------------------------------------------------------------------
// 		コンストラクタ
//-----------------------------------------------------------------------------
AnimationController::AnimationController(GameObject* obj)
	: Component(obj)
{
	m_ComponentName = "AnimationController";
}

//-----------------------------------------------------------------------------
// 		デストラクタ
//-----------------------------------------------------------------------------
AnimationController::~AnimationController()
{
	Term();
}

//-----------------------------------------------------------------------------
// 		初期化処理 
//-----------------------------------------------------------------------------
bool AnimationController::Init()
{
	if (!m_GameObject) return false;
	return Init(m_GameObject->GetModelPath());
}

//-----------------------------------------------------------------------------
// 		初期化処理 (パス指定)
//-----------------------------------------------------------------------------
bool AnimationController::Init(const std::wstring& modelPath)
{
	m_ModelPath = modelPath;
	m_CurrentAnimationName.clear();

	// スケルトン情報の取得
	m_pSkeleton = AnimationManager::GetInstance().GetSkeleton(m_ModelPath);
	if (m_pSkeleton == nullptr)
	{
		DLOG("Error: Skeleton not found for model: %ls", m_ModelPath.c_str());
		return false;
	}

	// ボーンマトリックスの初期化
	size_t boneCount = m_pSkeleton->bones.size();
	m_BoneMatrices.assign(boneCount, DirectX::XMMatrixIdentity());

	// 定数バッファの初期化
	if (!m_BoneMatrixCB.Init(GetDevice().Get(), GetPool(POOL_TYPE_RES), sizeof(BoneMatrixBuffer)))
	{
		ELOG("Failed to initialize bone matrix constant buffer");
		return false;
	}

	m_CurrentTime = 0.0f;
	m_IsPlaying = false;
	m_IsLoop = true;
	m_Speed = 1.0f;

	// 初期姿勢の計算とバッファ更新
	CalculateBoneMatrices();
	UpdateConstantBuffer();

	m_IsInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// 		終了処理
//-----------------------------------------------------------------------------
void AnimationController::Term()
{
	m_BoneMatrixCB.Term();
	m_BoneMatrices.clear();
	m_ChannelByBoneName.clear();
	m_pCurrentClip = nullptr;
	m_pSkeleton = nullptr;
	m_IsInitialized = false;
}

//-----------------------------------------------------------------------------
// 		更新処理
//-----------------------------------------------------------------------------
void AnimationController::Update(float deltaTime)
{
	if (!m_IsInitialized) return;

	// アニメーション時間の更新
	if (m_IsPlaying && m_pCurrentClip != nullptr)
	{
		if (m_pCurrentClip->ticksPerSecond <= 0.0f)
		{
			m_IsPlaying = false;
			return;
		}

		m_CurrentTime += deltaTime * m_Speed;
		float duration = m_pCurrentClip->duration / m_pCurrentClip->ticksPerSecond;

		if (m_IsLoop)
		{
			m_CurrentTime = fmodf(m_CurrentTime, duration);
		}
		else if (m_CurrentTime >= duration)
		{
			m_CurrentTime = duration;
			m_IsPlaying = false;
		}
	}

	CalculateBoneMatrices();
	UpdateConstantBuffer();
}

//-----------------------------------------------------------------------------
//      アニメーションを再生
//-----------------------------------------------------------------------------
bool AnimationController::Play(const std::string& animationName)
{
	if (!m_IsInitialized) return false;

	const AnimationClip* clip = AnimationManager::GetInstance().GetAnimationByName(animationName);
	if (!clip)
	{
		ELOG("AnimationController::Play: Animation '%s' not found", animationName.c_str());
		return false;
	}

	m_HoldPaused = false;
	m_pCurrentClip = clip;
	m_CurrentAnimationName = animationName;
	m_CurrentTime = 0.0f;
	m_IsPlaying = true;

	RebuildChannelLookup();

	CalculateBoneMatrices();
	UpdateConstantBuffer();

	return true;
}

//-----------------------------------------------------------------------------
//      アニメーションを停止
//-----------------------------------------------------------------------------
void AnimationController::Stop()
{
	m_HoldPaused = false;
	m_IsPlaying = false;
	m_CurrentTime = 0.0f;
	m_pCurrentClip = nullptr;
	m_CurrentAnimationName.clear();
	m_ChannelByBoneName.clear();

	CalculateBoneMatrices();
	UpdateConstantBuffer();
}

//-----------------------------------------------------------------------------
//		アニメーションを一時停止
//-----------------------------------------------------------------------------
void AnimationController::Pause()
{
	m_IsPlaying = false;
	m_HoldPaused = true;
}

//-----------------------------------------------------------------------------
// 	    一時停止解除
//-----------------------------------------------------------------------------
void AnimationController::Resume()
{
	m_HoldPaused = false;
	if (m_pCurrentClip) m_IsPlaying = true;
}

//-----------------------------------------------------------------------------
// 	    アニメーションクリップを直接設定
//-----------------------------------------------------------------------------
void AnimationController::SetAnimationClip(const std::wstring& modelPath, const std::string& clipName)
{
	m_ModelPath = modelPath;
	m_pCurrentClip = AnimationManager::GetInstance().GetAnimationClip(modelPath, clipName);

	RebuildChannelLookup();
	m_CurrentTime = 0.0f;
}

//-----------------------------------------------------------------------------
// 		ボーンマトリックスを計算
//-----------------------------------------------------------------------------
void AnimationController::CalculateBoneMatrices()
{
	if (!m_pSkeleton) return;

	// アニメーションがない場合はバインドポーズ
	if (m_pCurrentClip == nullptr)
	{
		for (size_t i = 0; i < m_BoneMatrices.size(); ++i)
		{
			const auto& bone = m_pSkeleton->bones[i];
			if (bone.hasBind)
			{
				m_BoneMatrices[i] = DirectX::XMMatrixTranspose(bone.boneBindGlobal * bone.boneBindInv);
			}
		}
		return;
	}

	const float tickTime = m_CurrentTime * m_pCurrentClip->ticksPerSecond;

	for (size_t i = 0; i < m_pSkeleton->bones.size(); ++i)
	{
		const auto& bone = m_pSkeleton->bones[i];
		if (!bone.hasBind) continue;

		const AnimationChannel* ch = nullptr;
		auto it = m_ChannelByBoneName.find(bone.name);
		if (it != m_ChannelByBoneName.end()) ch = it->second;

		if (ch && !ch->matrixKeys.empty())
		{
			DirectX::XMMATRIX currentMatrixKey = SampleGlobalMatrixNearest(*ch, tickTime);
			m_BoneMatrices[i] = DirectX::XMMatrixTranspose(bone.boneBindInv * currentMatrixKey);
		}
		else
		{
			m_BoneMatrices[i] = DirectX::XMMatrixTranspose(bone.boneBindInv * bone.boneBindGlobal);
		}
	}
}

//-----------------------------------------------------------------------------
// 		ルックアップの再構築
//-----------------------------------------------------------------------------
void AnimationController::RebuildChannelLookup()
{
	m_ChannelByBoneName.clear();
	if (!m_pCurrentClip) return;

	for (const auto& c : m_pCurrentClip->channels)
	{
		m_ChannelByBoneName[c.boneName] = &c;
	}
}

//-----------------------------------------------------------------------------
// 		定数バッファの更新
//-----------------------------------------------------------------------------
void AnimationController::UpdateConstantBuffer()
{
	struct alignas(128) BoneMatrixBufferInternal
	{
		DirectX::XMFLOAT4X4 matrices[kMaxBones];
	};

	BoneMatrixBufferInternal buffer = {};
	size_t count = min(m_BoneMatrices.size(), kMaxBones);

	for (size_t i = 0; i < count; ++i)
	{
		DirectX::XMStoreFloat4x4(&buffer.matrices[i], m_BoneMatrices[i]);
	}

	if (void* ptr = m_BoneMatrixCB.GetPtr())
	{
		memcpy(ptr, &buffer, sizeof(BoneMatrixBufferInternal));
	}
}