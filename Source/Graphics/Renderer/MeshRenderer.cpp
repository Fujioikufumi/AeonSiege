#include "MeshRenderer.h"
#include "Logger.h"
#include "AnimationController.h"
#include "Texture.h"
#include "GameObject.h"

//-----------------------------------------------------------------------------
// 	       コンストラクタ
//-----------------------------------------------------------------------------
MeshRenderer::MeshRenderer(GameObject* obj)
	: Component(obj)
{
	m_ComponentName = "MeshRenderer";
}

//-----------------------------------------------------------------------------
// 	       モデル読み込み
//-----------------------------------------------------------------------------
bool MeshRenderer::Load(const std::wstring& filePath, const std::wstring& pipelineName)
{
	m_ModelPath = filePath;
	std::wstring finalPipelineName = pipelineName;

	// レンダーシステムへの参照を保存
	m_RenderSystem = ResourceManager::GetInstance().GetRenderSystem();
	if (m_RenderSystem == nullptr)
	{
		ELOG("Error : RenderSystem is null");
		return false;
	}

	// MeshCBの初期化
	auto device = GetDevice();
	auto pool = GetPool(POOL_TYPE_RES);
	if (!m_MeshCB.Init(device.Get(), pool, sizeof(CbMesh)))
	{
		ELOG("Error : MeshCB::Init() Failed");
		return false;
	}

	// ModelManagerでモデルを読み込む
	if (!ModelManager::GetInstance().LoadModel(m_ModelPath))
	{
		ELOG("Error : Failed to load model. path = %ls", m_ModelPath.c_str());
		return false;
	}

	// モデル情報を取得してアニメーション有無を確認
	ModelInfo* modelInfo = ModelManager::GetInstance().GetModel(m_ModelPath);
	if (modelInfo != nullptr)
	{
		// アニメーションがある場合はスキニングパイプラインを優先する
		if (modelInfo->IsSkinned())
		{
			finalPipelineName = L"SkinnedFBXPipeline";
			m_HasAnimation = true;
		}
		else
		{
			// 指定がない場合はデフォルトのパイプライン
			if (finalPipelineName.empty()) finalPipelineName = L"ModelPipeline";
			m_HasAnimation = false;
		}
	}

	// パイプラインを登録
	if (!m_RenderSystem->SetupModelPipeline(m_ModelPath, finalPipelineName))
	{
		ELOG("Error : Failed to setup model pipeline for %ls", m_ModelPath.c_str());
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 	       終了処理
//-----------------------------------------------------------------------------
void MeshRenderer::Term()
{
	m_MeshCB.Term();
	m_RenderSystem = nullptr;
}

//-----------------------------------------------------------------------------
// 	       更新処理
//-----------------------------------------------------------------------------
void MeshRenderer::Update(float deltaTime)
{
	if (m_GameObject == nullptr) return;

	// GameObjectからWorldMatrixを取得してMeshCBを更新
	DirectX::XMMATRIX worldMatrix = m_GameObject->GetWorldMatrix();
	auto meshCB = m_MeshCB.GetPtr<CbMesh>();
	if (meshCB != nullptr)
	{
		DirectX::XMStoreFloat4x4(&meshCB->World, DirectX::XMMatrixTranspose(worldMatrix));
	}
}


//-----------------------------------------------------------------------------
// 	       描画処理
//-----------------------------------------------------------------------------
void MeshRenderer::Draw(const RenderContext& context)
{
    if (m_RenderSystem == nullptr || m_ModelPath.empty()) return;

    // モデル情報を取得
    ModelInfo* modelInfo = ModelManager::GetInstance().GetModel(m_ModelPath);

    if(modelInfo == nullptr)
    {
        ELOG("Error : ModelInfo is null. path = %ls", m_ModelPath.c_str());
        return;
	}

    // MeshCBのハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle = m_MeshCB.GetHandleGPU();

	// GameObjectのワールド行列を取得
    XMMATRIX worldMatrix = XMMatrixIdentity();
    if (m_GameObject != nullptr)
    {
		worldMatrix = m_GameObject->GetWorldMatrix();
    }
    // アニメーションがある場合はボーンマトリックスを設定
    D3D12_GPU_DESCRIPTOR_HANDLE boneMatrixCBHandle = {};
    if (m_HasAnimation && m_GameObject != nullptr)
    {
		AnimationController* animationController = m_GameObject->GetComponent<AnimationController>();
        if (animationController != nullptr && animationController->IsInitialized())
        {
            
            boneMatrixCBHandle = animationController->GetBoneMatrixCBHandle();
        }
    }

    // RenderSystem経由で描画（MeshCBを使用）
	if (!m_RenderSystem->DrawModelAllMeshes(
		context,
		m_ModelPath,
		worldMatrix,
		meshCBHandle,
		boneMatrixCBHandle
	))
	{
		ELOG("Error : Failed to draw model. path = %ls", m_ModelPath.c_str());
	}
}
