//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "RenderSystem.h"
#include "Logger.h"
#include "NameSpace.h"
#include "ResourceManager.h"
#include "MeshRenderer.h"

//-----------------------------------------------------------------------------
//          コンストラクタ
//-----------------------------------------------------------------------------
RenderSystem::RenderSystem()
    : m_Device(nullptr)
    , m_Pool{}
    , m_Queue(nullptr)
    , m_PipelineManager(nullptr)
    , m_ShaderManager(nullptr)
    , m_ModelManager(nullptr)
{
}

//-----------------------------------------------------------------------------
//          デストラクタ
//-----------------------------------------------------------------------------
RenderSystem::~RenderSystem()
{
    Term();
}

//-----------------------------------------------------------------------------
//          初期化処理
//-----------------------------------------------------------------------------
bool RenderSystem::Init()
{
    auto& resourceManager = ResourceManager::GetInstance();

    m_Device = resourceManager.GetDevice();
    m_Queue = resourceManager.GetQueue();

    for (int i = 0; i < POOL_COUNT; ++i)
    {
        m_Pool[i] = resourceManager.GetPool(static_cast<POOL_TYPE>(i));
    }

	// デバイスとキューの確認
    if (!m_Device || !m_Queue)
    {
        ELOG("Error : ResourceManager not initialized");
        return false;
    }

	// プールの確認
    for (int i = 0; i < POOL_COUNT; ++i)
    {
        if (m_Pool[i] == nullptr)
        {
            ELOG("Error : DescriptorPool[%d] is null", i);
            return false;
        }
    }


	// マネージャーの取得
    m_PipelineManager = &PipelineStateManager::GetInstance();
    m_ShaderManager = &ShaderManager::GetInstance();
    m_ModelManager = &ModelManager::GetInstance();

    DLOG("RenderSystem initialized successfully");
    return true;
}

//-----------------------------------------------------------------------------
//          終了処理
//-----------------------------------------------------------------------------
void RenderSystem::Term()
{
    m_ModelPipelineMap.clear();
    m_Device.Reset();
    m_Queue.Reset();

    for (int i = 0; i < POOL_COUNT; ++i)
    {
        m_Pool[i] = nullptr;
    }

    m_PipelineManager = nullptr;
    m_ShaderManager = nullptr;
    m_ModelManager = nullptr;

    DLOG("RenderSystem terminated");
}

//-----------------------------------------------------------------------------
//          パイプラインステートの設定
//-----------------------------------------------------------------------------
bool RenderSystem::SetupModelPipeline(
    const std::wstring& modelPath,
    const std::wstring& pipelineName)
{
	// パイプラインステートが存在するか確認
    if (!m_PipelineManager->IsPipelineStateCreated(pipelineName))
    {
        ELOG("Error : Pipeline state not found. pipelineName = %ls", pipelineName.c_str());
        return false;
    }

	// モデルパスにパイプライン名を紐付け
    m_ModelPipelineMap[modelPath] = pipelineName;

    DLOG("Model pipeline setup: %ls -> %ls", modelPath.c_str(), pipelineName.c_str());
    return true;
}

//-----------------------------------------------------------------------------
//          テクスチャ設定
//-----------------------------------------------------------------------------
bool RenderSystem::BindMaterialTextures(
    ID3D12GraphicsCommandList* pCmdList,
    Material* material,
    int materialId)
{
    if (material == nullptr)
    {
        ELOG("Error : Material is null");
        return false;
    }

    // BaseColorテクスチャ（ルートパラメータ6）
    D3D12_GPU_DESCRIPTOR_HANDLE handleBase = material->GetTextureHandle(materialId, TU_BASE_COLOR);
    if (handleBase.ptr == 0)
    {
        handleBase = material->GetTextureHandle(0, TU_BASE_COLOR);
    }

    if (handleBase.ptr != 0)
    {
        
        pCmdList->SetGraphicsRootDescriptorTable(6, handleBase);
    }

    // Metallicテクスチャ（ルートパラメータ7）
    D3D12_GPU_DESCRIPTOR_HANDLE handleMetal = material->GetTextureHandle(materialId, TU_METALLIC);
    if (handleMetal.ptr == 0)
    {
        handleMetal = material->GetTextureHandle(0, TU_METALLIC);
    }
    if (handleMetal.ptr != 0)
    {
        pCmdList->SetGraphicsRootDescriptorTable(7, handleMetal);
    }

    // Roughnessテクスチャ（ルートパラメータ8）
    D3D12_GPU_DESCRIPTOR_HANDLE handleRough = material->GetTextureHandle(materialId, TU_ROUGHNESS);
    if (handleRough.ptr == 0)
    {
        handleRough = material->GetTextureHandle(0, TU_ROUGHNESS);
    }
    if (handleRough.ptr != 0)
    {
        pCmdList->SetGraphicsRootDescriptorTable(8, handleRough);
    }

    // Normalテクスチャ（ルートパラメータ9）
    D3D12_GPU_DESCRIPTOR_HANDLE handleNormal = material->GetTextureHandle(materialId, TU_NORMAL);
    if (handleNormal.ptr == 0)
    {
        handleNormal = material->GetTextureHandle(0, TU_NORMAL);
    }
    if (handleNormal.ptr != 0)
    {
        pCmdList->SetGraphicsRootDescriptorTable(9, handleNormal);
    }

    return true;
}

//-----------------------------------------------------------------------------
//          パイプラインステートの設定
//-----------------------------------------------------------------------------
bool RenderSystem::SetPipelineState(
    const RenderContext& context,
    const std::wstring& pipelineName)
{
	// パイプラインステート情報を取得
    PipelineStateInfo* pipeline = m_PipelineManager->GetPipelineState(pipelineName);
    if (pipeline == nullptr)
    {
        ELOG("Error : Pipeline state not found. pipelineName = %ls", pipelineName.c_str());
        return false;
    }

    context.pCmdList->SetGraphicsRootSignature(pipeline->rootSig.GetPtr());
    context.pCmdList->SetPipelineState(pipeline->pPSO.Get());
    context.pCmdList->RSSetViewports(1, &context.viewport);
    context.pCmdList->RSSetScissorRects(1, &context.scissorRect);

    // パイプラインステートに応じてルートパラメータを設定
    if (pipelineName == L"ModelPipeline")
    {
        context.pCmdList->SetGraphicsRootDescriptorTable(0, context.transformCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(2, context.lightCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(3, context.cameraCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(5, context.shadowCB);    // register(b3)
        context.pCmdList->SetGraphicsRootDescriptorTable(10, context.shadowMapSRV); // register(t4)
    }
    else if (pipelineName == L"GrassNear")
    {
        context.pCmdList->SetGraphicsRootDescriptorTable(0, context.transformCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(1, context.cameraCB);
    }
    else if(pipelineName == L"SkinnedModelPipeline")
    {
        // SkinnedModelPipeline用の設定（影なし、スキニングあり）
        context.pCmdList->SetGraphicsRootDescriptorTable(0, context.transformCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(3, context.lightCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(4, context.cameraCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(10, context.shadowCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(11, context.shadowMapSRV);
    }
    else if (pipelineName == L"SkinnedFBXPipeline")
    {
        context.pCmdList->SetGraphicsRootDescriptorTable(0, context.transformCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(3, context.lightCB);
        context.pCmdList->SetGraphicsRootDescriptorTable(4, context.cameraCB);
    }
    return true;
}

//-----------------------------------------------------------------------------
//         メッシュ描画
//-----------------------------------------------------------------------------
bool RenderSystem::DrawMesh(
    const RenderContext& context,
    Mesh* mesh,
    Material* material,
    const XMMATRIX& worldMatrix,
    int materialId,
    D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle)
{
    if (mesh == nullptr)
    {
        ELOG("Error : Mesh is nullptr");
        return false;
    }

    context.pCmdList->SetGraphicsRootDescriptorTable(1, meshCBHandle);

    if (material != nullptr)
    {
        BindMaterialTextures(context.pCmdList, material, materialId);
    }

    mesh->Draw(context.pCmdList);

    return true;
}

//-----------------------------------------------------------------------------
//          モデル描画
//-----------------------------------------------------------------------------
bool RenderSystem::DrawModel(
    const RenderContext& context,
    const std::wstring& modelPath,
    const XMMATRIX& worldMatrix,
    int materialId)
{
	// モデル情報を取得
    ModelInfo* modelInfo = m_ModelManager->GetModel(modelPath);
    if (modelInfo == nullptr)
    {
        ELOG("Error : Model not found. modelPath = %ls", modelPath.c_str());
        return false;
    }

    if (modelInfo->meshes.empty())
    {
        ELOG("Error : Model has no meshes. modelPath = %ls", modelPath.c_str());
        return false;
    }

	// パイプラインステート名を取得
    std::wstring pipelineName = L"ModelPipeline";
    auto it = m_ModelPipelineMap.find(modelPath);
    if (it != m_ModelPipelineMap.end())
    {
        pipelineName = it->second;
    }

	// パイプラインステートを設定
    if (!SetPipelineState(context, pipelineName))
    {
        return false;
    }

	// 最初のメッシュを描画
    Mesh* mesh = modelInfo->meshes[0];
    Material* material = &modelInfo->material;
    BindMaterialTextures(context.pCmdList, material, materialId);

	// メッシュ描画
    mesh->Draw(context.pCmdList);

    return true;
}

//-----------------------------------------------------------------------------
// 		モデルの全メッシュ描画
//-----------------------------------------------------------------------------
bool RenderSystem::DrawModelAllMeshes(
    const RenderContext& context,
    const std::wstring& modelPath,
    const XMMATRIX& worldMatrix,
    D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE boneMatrixCBHandle)
{
	// モデル情報を取得
    ModelInfo* modelInfo = m_ModelManager->GetModel(modelPath);
    if (modelInfo == nullptr)
    {
        ELOG("Error : Model not found. modelPath = %ls", modelPath.c_str());
        return false;
    }

    if (modelInfo->meshes.empty())
    {
        ELOG("Error : Model has no meshes. modelPath = %ls", modelPath.c_str());
        return false;
    }

	//  パイプラインステート名を取得
    std::wstring pipelineName = L"ModelPipeline";
    auto it = m_ModelPipelineMap.find(modelPath);
    if (it != m_ModelPipelineMap.end())
    {
        pipelineName = it->second;
    }

    // アニメーションがある場合はスキニングパイプラインを使用
    if (modelInfo->IsSkinned())
    {
        pipelineName = L"SkinnedFBXPipeline";
    }

	// パイプラインステートを設定
    if (!SetPipelineState(context, pipelineName))
    {
        return false;
    }

	// メッシュ共通のCBVを設定
    if (meshCBHandle.ptr != 0)
    {
        context.pCmdList->SetGraphicsRootDescriptorTable(1, meshCBHandle);
    }

    // ボーンマトリックスCBVを設定
    if (boneMatrixCBHandle.ptr != 0 && modelInfo->HasAnimation())
    {
        // AnimationControllerから取得したボーンマトリックスを使用
        context.pCmdList->SetGraphicsRootDescriptorTable(2, boneMatrixCBHandle);
    }
    else if (modelInfo->IsSkinned())
    {
        // アニメーションはないがスキングメッシュの場合、または
        ConstantBuffer* pDefaultBoneMatrixCB = modelInfo->GetDefaultBoneMatrixCB();
        if (pDefaultBoneMatrixCB != nullptr)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE defaultBoneMatrixCBHandle = pDefaultBoneMatrixCB->GetHandleGPU();
            if (defaultBoneMatrixCBHandle.ptr != 0)
            {
                context.pCmdList->SetGraphicsRootDescriptorTable(2, defaultBoneMatrixCBHandle);
            }
            else
            {
                ELOG("RenderSystem::DrawModelAllMeshes: Default bone matrix CB handle is invalid for skinned mesh");
            }
        }
        else
        {
            ELOG("RenderSystem::DrawModelAllMeshes: Default bone matrix CB is null for skinned mesh");
        }
    }

    Material* material = &modelInfo->material;

	// 全メッシュを描画
    for (size_t i = 0; i < modelInfo->meshes.size(); ++i)
    {
        Mesh* mesh = modelInfo->meshes[i];
		// マテリアルIDを取得
        int matId = static_cast<int>(mesh->GetMaterialId());
        // マテリアルCBVを設定（パイプラインに応じてルートパラメータ番号を変更）
        D3D12_GPU_DESCRIPTOR_HANDLE materialCBHandle = material->GetBufferHandle(matId);
        if (materialCBHandle.ptr != 0)
        {
            // SkinnedModelPipelineの場合はルート5、ModelPipelineの場合はルート4
            int materialRootParam =
                (pipelineName == L"SkinnedModelPipeline" || pipelineName == L"SkinnedFBXPipeline")
                ? 5
                : 4;
            context.pCmdList->SetGraphicsRootDescriptorTable(materialRootParam, materialCBHandle);
        }
		// テクスチャをバインド
        BindMaterialTextures(context.pCmdList, material, matId);

		// メッシュ描画
        mesh->Draw(context.pCmdList);
    }
    return true;
}

//-----------------------------------------------------------------------------
// 		モデルにパイプラインが設定されているか確認
//-----------------------------------------------------------------------------
bool RenderSystem::IsModelPipelineSetup(const std::wstring& modelPath) const
{
    return m_ModelPipelineMap.find(modelPath) != m_ModelPipelineMap.end();
}