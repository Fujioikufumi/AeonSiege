#include "AnimationManager.h"
#include "Logger.h"
#include "FileUtil.h"
#include <algorithm>
#include "BinaryAnimLoader.h"
#include "ModelFormat.h"
#include <fstream>
//-----------------------------------------------------------------------------
// 		初期化処理
//-----------------------------------------------------------------------------
bool AnimationManager::Init()
{
	m_Skeletons.clear();
	m_Animations.clear();
	m_NamedAnimations.clear();
	m_AnimationBoneNames.clear();
	return true;
}

//-----------------------------------------------------------------------------
// 		終了処理
//-----------------------------------------------------------------------------
void AnimationManager::Term()
{
	m_Skeletons.clear();
	m_Animations.clear();
	m_NamedAnimations.clear();
	m_AnimationBoneNames.clear();
}

//-----------------------------------------------------------------------------
//		アニメーションファイルのロード
//-----------------------------------------------------------------------------
bool AnimationManager::LoadAnimation(const std::wstring& animationFilePath, const std::string& animationName)
{
	// 既に同じ名前で登録されていないか確認
	if (m_NamedAnimations.find(animationName) != m_NamedAnimations.end())
	{
		DLOG("AnimationManager::LoadAnimation: Animation name '%s' already exists", animationName.c_str());
		return true;
	}

	// ファイルパスの解決
	std::wstring fullPath;
	if (!SearchFilePath(animationFilePath.c_str(), fullPath))
	{
		ELOG("AnimationManager::LoadAnimation: Animation file not found: %ls", animationFilePath.c_str());
		return false;
	}

	std::wstring ext = GetFileExtension(fullPath);

	// banm 以外の拡張子ならエラーを出して終了
	if (ext != L".banm" && ext != L".BANM")
	{
		ELOG("Unsupported animation format: %ls", fullPath.c_str());
		return false;
	}

	BinaryAnimLoader loader;
	std::vector<ResMesh> resMesh;
	std::vector<ResMaterial> resMaterial;
	std::vector<AnimationClip> animations;

	if (!loader.Load(fullPath.c_str(), resMesh, resMaterial, nullptr, &animations))
	{
		ELOG("AnimationManager::LoadAnimation: Failed to load animation file: %ls", fullPath.c_str());
		return false;
	}

	if (animations.empty())
	{
		ELOG("AnimationManager::LoadAnimation: No animations found in file: %ls", fullPath.c_str());
		return false;
	}

	AnimationClip& clip = animations[0];

	// アニメーションクリップからボーン名を抽出
	std::set<std::string> boneNames;
	for (const auto& channel : clip.channels)
	{
		boneNames.insert(channel.boneName);
	}

	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		// 既に同じ名前で登録されていないか再確認
		if (m_NamedAnimations.find(animationName) == m_NamedAnimations.end())
		{
			// 名前で登録
			m_NamedAnimations[animationName] = clip;
			m_AnimationBoneNames[animationName] = boneNames;
		}
	} 

	DLOG("AnimationManager::LoadAnimation: Registered animation '%s' with %zu bones",
		animationName.c_str(), boneNames.size());

	return true;
}

//-----------------------------------------------------------------------------
// 		登録済みアニメーションを名前で取得
//-----------------------------------------------------------------------------
const AnimationClip* AnimationManager::GetAnimationByName(const std::string& animationName) const
{
	auto it = m_NamedAnimations.find(animationName);
	if(it != m_NamedAnimations.end())
	{
		return &(it->second);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
//		アニメーションに含まれるボーン名一覧を取得
//-----------------------------------------------------------------------------
const std::set<std::string>* AnimationManager::GetAnimationBoneNames(const std::string& animationName) const
{
	auto it = m_AnimationBoneNames.find(animationName);
	if (it != m_AnimationBoneNames.end())
	{
		return &(it->second);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
//		ボーン階層情報登録
//-----------------------------------------------------------------------------
void AnimationManager::RegisterSkeleton(const std::wstring& modelPath, const SkeletonInfo& skeleton)
{
	m_Skeletons[modelPath] = skeleton;
}

//-----------------------------------------------------------------------------
//		アニメーションクリップ登録
//-----------------------------------------------------------------------------
void AnimationManager::RegisterAnimations(const std::wstring& modelPath, const std::vector<AnimationClip>& animations)
{
	m_Animations[modelPath] = animations;
}

//-----------------------------------------------------------------------------
//		スケルトン情報取得
//-----------------------------------------------------------------------------
const SkeletonInfo* AnimationManager::GetSkeleton(const std::wstring& modelPath) const
{
	auto it = m_Skeletons.find(modelPath);
	if (it != m_Skeletons.end())
	{
		return &(it->second);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
//		アニメーション取得
//-----------------------------------------------------------------------------
const std::vector<AnimationClip>* AnimationManager::GetAnimations(const std::wstring& modelPath) const
{
	auto it = m_Animations.find(modelPath);
	if (it != m_Animations.end())
	{
		return &(it->second);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
//　		アニメーションクリップ取得
//-----------------------------------------------------------------------------
const AnimationClip* AnimationManager::GetAnimationClip(const std::wstring& modelPath, const std::string& clipName) const
{
	const auto* animations = GetAnimations(modelPath);
	if (animations != nullptr)
	{
		for (const auto& clip : *animations)
		{
			if (clip.name == clipName)
				return &clip;
		}
	}


	const AnimationClip* byName = GetAnimationByName(clipName);
	if (byName != nullptr)
		return byName;
	return nullptr;
}

//-----------------------------------------------------------------------------
//		登録済みアニメーション名の一覧を取得
//-----------------------------------------------------------------------------
std::vector<std::string> AnimationManager::GetAvailableAnimationNames() const
{
	std::vector<std::string> names;
	names.reserve(m_NamedAnimations.size());
	for (const auto& pair : m_NamedAnimations)
	{
		names.push_back(pair.first);
	}
	return names;
}

//-----------------------------------------------------------------------------
//		モデルアンロード時: スケルトンとパス別アニメ一覧の登録解除
//-----------------------------------------------------------------------------
void AnimationManager::UnregisterModel(const std::wstring& modelPath)
{
	m_Skeletons.erase(modelPath);
	m_Animations.erase(modelPath);
}
