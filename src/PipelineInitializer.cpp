#include "PipelineInitializer.h"
#include "ShaderManager.h"
#include "PipelineStateManager.h"
#include "CommonStates.h"
#include "Logger.h"
#include <cstddef>
#include "ResMesh.h"

//-----------------------------------------------------------------------------
// 	    全てのパイプラインを初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitializeAllPipelines()
{
	// PipelineStateManager初期化
	if (!PipelineStateManager::GetInstance().Init())
	{
		ELOG("Error : PipelineStateManager::Init() Failed");
		return false;
	}

	// モデルパイプライン初期化
	if (!InitializeModelPipeline())
	{
		ELOG("Error : InitializeModelPipeline() Failed");
		return false;
	}

	// スキンドFBXパイプライン初期化
	if (!InitializeSkinnedFBXPipeline())
	{
		ELOG("Error : InitializeSkinnedFBXPipeline() Failed");
		return false;
	}
	
	// UIパイプライン初期化
	if (!InitializeUIPipeline())
	{
		ELOG("Error : InitializeUIPipeline() Failed");
		return false;
	}

	// シャドウパイプライン初期化
	if (!InitializeShadowPipeline())
	{
		ELOG("Error : InitializeShadowPipeline() Failed");
		return false;
	}

	// 地形パイプライン初期化
	if(!InitalizeTerrainPipeline())
	{
		ELOG("Error : InitalizeTerrainpipeline() Failed");
		return false;
	}

	// 草原パイプライン初期化
	if(!InitalizeMeadowGSPipeline())
	{
		ELOG("Error : InitalizeMeadowGSPipeline() Failed");
		return false;
	}

	if (!InitializeGrassGeneratorPipeline())
	{
		ELOG("Error : InitializeGrassGeneratorPipeline() Failed");
		return false;
	}

	if (!InitalizeMaskedUIPipeline())
	{
		ELOG("Error : InitalizeMaskedUIPipeline() Failed");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 	    モデルパイプライン初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitializeModelPipeline()
{
	if (!ShaderManager::GetInstance().LoadShader(L"Basic", L"BasicVS.cso", L"BasicPS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed");
		return false;
	}

	// ルートシグネチャ設定
	RootSignatureConfig config;
	// CBV設定
	config.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 }); // CbTransform
	config.cbvConfigs.push_back({ ShaderStage::VS, 1, 1 }); // CbMesh
	config.cbvConfigs.push_back({ ShaderStage::PS, 2, 1 }); // CbLight
	config.cbvConfigs.push_back({ ShaderStage::PS, 3, 2 }); // CbCamera
	config.cbvConfigs.push_back({ ShaderStage::PS, 4, 4 }); // CbMaterial

	// SRV設定（インデックスを6から開始）
	config.srvConfigs.push_back({ ShaderStage::PS, 6, 0 }); // BaseColor（ルートパラメータインデックス6）
	config.srvConfigs.push_back({ ShaderStage::PS, 7, 1 }); // Metallic（ルートパラメータインデックス7）
	config.srvConfigs.push_back({ ShaderStage::PS, 8, 2 }); // Roughness（ルートパラメータインデックス8）
	config.srvConfigs.push_back({ ShaderStage::PS, 9, 3 }); // Normal（ルートパラメータインデックス9）

	// サンプラー設定
	config.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearWrap });
	config.samplerConfigs.push_back({ ShaderStage::PS, 1, SamplerState::LinearWrap });
	config.samplerConfigs.push_back({ ShaderStage::PS, 2, SamplerState::LinearWrap });
	config.samplerConfigs.push_back({ ShaderStage::PS, 3, SamplerState::LinearWrap });

	config.allowIL = true;
	config.allowSO = false;

	// パイプラインステート設定
	PipelineStateDesc pipelineDesc;
	pipelineDesc.shaderName = L"Basic";
	pipelineDesc.inputLayout = GetModelInputLayout();
	pipelineDesc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	pipelineDesc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineDesc.rasterizerState = DirectX::CommonStates::CullNone;
	pipelineDesc.blendState = DirectX::CommonStates::AlphaBlend; // アルファブレンドを使用
	pipelineDesc.depthStencilState = DirectX::CommonStates::DepthDefault;
	pipelineDesc.sampleCount = 1;
	pipelineDesc.sampleQuality = 0;
	pipelineDesc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"ModelPipeline",
		pipelineDesc,
		&config))
	{
		ELOG("Error : Failed to create ModelPipeline");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 	    スキニングFBXパイプライン初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitializeSkinnedFBXPipeline()
{
	if (!ShaderManager::GetInstance().LoadShader(L"SkinnedFBX", L"SkinnedFBXVS.cso", L"SkinnedFBXPS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for SkinnedFBX");
		return false;
	}

	RootSignatureConfig rs;
	rs.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 }); // b0 Transform
	rs.cbvConfigs.push_back({ ShaderStage::VS, 1, 1 }); // b1 Mesh(World)
	rs.cbvConfigs.push_back({ ShaderStage::VS, 2, 2 }); // b2 BonePalette

	rs.cbvConfigs.push_back({ ShaderStage::PS, 3, 1 }); // b1 Light
	rs.cbvConfigs.push_back({ ShaderStage::PS, 4, 2 }); // b2 Camera
	rs.cbvConfigs.push_back({ ShaderStage::PS, 5, 4 }); // b4 Material

	rs.srvConfigs.push_back({ ShaderStage::PS, 6, 0 }); // t0 BaseColor
	rs.srvConfigs.push_back({ ShaderStage::PS, 7, 1 }); // t1 Metallic
	rs.srvConfigs.push_back({ ShaderStage::PS, 8, 2 }); // t2 Roughness
	rs.srvConfigs.push_back({ ShaderStage::PS, 9, 3 }); // t3 Normal

	rs.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearWrap });
	rs.samplerConfigs.push_back({ ShaderStage::PS, 1, SamplerState::LinearWrap });
	rs.samplerConfigs.push_back({ ShaderStage::PS, 2, SamplerState::LinearWrap });
	rs.samplerConfigs.push_back({ ShaderStage::PS, 3, SamplerState::LinearWrap });

	rs.allowIL = true;
	rs.allowSO = false;

	PipelineStateDesc desc;
	desc.shaderName = L"SkinnedFBX";
	desc.inputLayout = GetSkinnedModelInputLayout();
	desc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	desc.rasterizerState = DirectX::CommonStates::CullNone;
	desc.blendState = DirectX::CommonStates::Opaque;
	desc.depthStencilState = DirectX::CommonStates::DepthDefault;
	desc.sampleCount = 1;
	desc.sampleQuality = 0;
	desc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	if (!PipelineStateManager::GetInstance().CreatePipelineState(L"SkinnedFBXPipeline", desc, &rs))
	{
		ELOG("Error : Failed to create SkinnedFBXPipeline");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//    UIパイプラインの初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitializeUIPipeline()
{
	if (!ShaderManager::GetInstance().LoadShader(L"UITexture", L"TextureVS.cso", L"TexturePS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for UITexture");
		return false;
	}

	RootSignatureConfig uiRootSigConfig;
	uiRootSigConfig.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 });
	uiRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 1, 0 });
	uiRootSigConfig.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearClamp });
	uiRootSigConfig.allowIL = true;
	uiRootSigConfig.allowSO = false;

	PipelineStateDesc uiPipelineDesc;
	uiPipelineDesc.shaderName = L"UITexture";
	uiPipelineDesc.inputLayout = GetUIInputLayout();
	uiPipelineDesc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	uiPipelineDesc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	uiPipelineDesc.rasterizerState = DirectX::CommonStates::CullNone;
	uiPipelineDesc.blendState = DirectX::CommonStates::NonPremultiplied;
	uiPipelineDesc.depthStencilState = DirectX::CommonStates::DepthNone;
	uiPipelineDesc.sampleCount = 1;
	uiPipelineDesc.sampleQuality = 0;
	uiPipelineDesc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"UIPipeline",
		uiPipelineDesc,
		&uiRootSigConfig))
	{
		ELOG("Error : Failed to create UIPipeline");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//   シャドウパイプラインの初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitializeShadowPipeline()
{
	// シェーダー読み込み
	if (!ShaderManager::GetInstance().LoadShader(L"ShadowMap", L"ShadowMapVS.cso", L"ShadowMapPS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for ShadowMap");
		return false;
	}

	auto shadowInputLayout = GetModelInputLayout();

	// PlaneShadowPipeline
	RootSignatureConfig planeShadowConfig;
	planeShadowConfig.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 });
	planeShadowConfig.cbvConfigs.push_back({ ShaderStage::PS, 1, 3 });
	planeShadowConfig.srvConfigs.push_back({ ShaderStage::PS, 2, 0 });
	planeShadowConfig.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearWrap });
	planeShadowConfig.srvConfigs.push_back({ ShaderStage::PS, 3, 4 });
	planeShadowConfig.shadowSamplerConfigs.push_back({ ShaderStage::PS, 4 });
	planeShadowConfig.allowIL = true;
	planeShadowConfig.allowSO = false;

	// ShadowPipeline
	RootSignatureConfig shadowConfig;
	shadowConfig.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 });
	shadowConfig.allowIL = true;
	shadowConfig.allowSO = false;

	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"ShadowPipeline",
		L"ShadowMap",
		shadowInputLayout,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_D32_FLOAT,
		&shadowConfig))
	{
		ELOG("Error : Failed to create ShadowPipeline");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 	 地形パイプラインの初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitalizeTerrainPipeline()
{
	// 1. シェーダーをロード
	if (!ShaderManager::GetInstance().LoadShader(L"Terrain", L"TerrainVS.cso", L"TerrainPS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for Terrain");
		return false;
	}

	// 2. ルートシグネチャ設定
	RootSignatureConfig terrainRootSigConfig;
	terrainRootSigConfig.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 });  // Transform (b0)
	terrainRootSigConfig.cbvConfigs.push_back({ ShaderStage::PS, 2, 1 });  // Light (b1)
	terrainRootSigConfig.cbvConfigs.push_back({ ShaderStage::PS, 3, 2 });  // Camera (b2)
	terrainRootSigConfig.cbvConfigs.push_back({ ShaderStage::PS, 4, 4 });  // Material (b4)

	// テクスチャSRV設定
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 6, 0 });  // BaseColorMap (t0)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 7, 1 });  // MetallicMap (t1)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 8, 2 });  // RoughnessMap (t2)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 9, 3 });  // NormalMap (t3)

	// フィールドテクスチャと地形テクスチャ用SRV設定
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 10, 4 }); // FieldMap (t4) - 色マップ
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 11, 5 }); // GrassTexture (t5)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 12, 6 }); // WaterTexture (t6)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 13, 7 }); // SandTexture (t7)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 14, 8 }); // RockTexture (t8)
	terrainRootSigConfig.srvConfigs.push_back({ ShaderStage::PS, 16, 10 }); // MacroTexture (t10) ←★これを追加
	
	// サンプラー設定
	terrainRootSigConfig.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearWrap });  // 共通サンプラー (s0)
	
	terrainRootSigConfig.allowIL = true;
	terrainRootSigConfig.allowSO = false;

	// 4. 通常パイプラインの設定
	PipelineStateDesc solidPipelineDesc;
	solidPipelineDesc.shaderName = L"Terrain";
	solidPipelineDesc.inputLayout = GetTerrainInputLayout();
	solidPipelineDesc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	solidPipelineDesc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	solidPipelineDesc.rasterizerState = DirectX::CommonStates::CullCounterClockwise;
	// solidPipelineDesc.rasterizerState = DirectX::CommonStates::Wireframe; // ワイヤーフレーム
	solidPipelineDesc.blendState = DirectX::CommonStates::Opaque;
	solidPipelineDesc.depthStencilState = DirectX::CommonStates::DepthDefault; // 無効/有効化しても描画されない
	solidPipelineDesc.sampleCount = 1;
	solidPipelineDesc.sampleQuality = 0;
	solidPipelineDesc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"TerrainPipeline",
		solidPipelineDesc,
		&terrainRootSigConfig))
	{
		ELOG("Error : Failed to create TerrainPipeline");
		return false;
	}


	// 地形頂点生成コンピュートシェーダー用パイプライン
	if (!ShaderManager::GetInstance().LoadComputeShader(L"TerrainVertexGen", L"TerrainVertexGenCS.cso"))
	{
		ELOG("Error : LoadComputeShader TerrainVertexGen Failed");
		return false;
	}

	RootSignatureConfig terrainGenCsConfig;
	terrainGenCsConfig.cbvConfigs.push_back({ ShaderStage::ALL, 0, 0 });   // b0
	terrainGenCsConfig.srvConfigs.push_back({ ShaderStage::ALL, 1, 0 });    // t0
	terrainGenCsConfig.samplerConfigs.push_back({ ShaderStage::ALL, 0, SamplerState::PointClamp }); // s0
	terrainGenCsConfig.uavConfigs.push_back({ ShaderStage::ALL, 2, 0 });    // u0
	terrainGenCsConfig.allowIL = false;
	terrainGenCsConfig.allowSO = false;

	if (!PipelineStateManager::GetInstance().CreateComputePipelineState(
		L"TerrainVertexGenPipeline",
		L"TerrainVertexGen",
		&terrainGenCsConfig))
	{
		ELOG("Error : CreateComputePipelineState TerrainVertexGenPipeline Failed");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 	 草原描画用ジオメトリシェーダーパイプラインの初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitalizeMeadowGSPipeline()
{
	if (!ShaderManager::GetInstance().LoadShader(
		L"GrassNear",
		L"GrassNearVS.cso",
		L"GrassNearPS.cso",
		L"GrassNearGS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for GrassNearGS");
		return false;
	}
	RootSignatureConfig config;
	config.cbvConfigs.push_back({ ShaderStage::ALL, 0, 0 }); // b0 View/Proj
	config.srvConfigs.push_back({ ShaderStage::PS, 1, 0 }); // t0 BaseColor
	config.cbvConfigs.push_back({ ShaderStage::GS, 2, 3 }); // b3 風用
	config.srvConfigs.push_back({ ShaderStage::PS, 3, 1 });
	config.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearClamp });
	config.allowIL = true;
	config.allowSO = false;
	PipelineStateDesc psoDesc;
	psoDesc.shaderName = L"GrassNear";
	psoDesc.inputLayout = GetMeadowGSInputLayout();
	psoDesc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	psoDesc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.rasterizerState = DirectX::CommonStates::CullNone;
	psoDesc.blendState = DirectX::CommonStates::AlphaBlend;
	psoDesc.depthStencilState = DirectX::CommonStates::DepthDefault;
	psoDesc.sampleCount = 1;
	psoDesc.sampleQuality = 0;
	psoDesc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"GrassNear",
		psoDesc,
		&config))
	{
		ELOG("Error : Failed to create GrassNearGS pipeline");
		return false;
	}
	return true;
}

bool PipelineInitializer::InitializeGrassGeneratorPipeline()
{
	if (!ShaderManager::GetInstance().LoadComputeShader(L"GrassGenerator", L"GrassGeneratorCS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for GrassGenerator");
		return false;
	}

	RootSignatureConfig config;
	config.cbvConfigs.push_back({ ShaderStage::ALL, 0, 0 });
	config.srvConfigs.push_back({ ShaderStage::ALL, 1, 0 });
	config.srvConfigs.push_back({ ShaderStage::ALL, 2, 1 });
	config.samplerConfigs.push_back({ ShaderStage::ALL, 0, SamplerState::LinearClamp });
	
	config.uavConfigs.push_back({ ShaderStage::ALL, 3, 0 });
	config.uavConfigs.push_back({ ShaderStage::ALL, 4, 1 });

	config.allowIL = false;
	config.allowSO = false;

	if (!PipelineStateManager::GetInstance().CreateComputePipelineState(
		L"GrassGeneratorPipeline",
		L"GrassGenerator",
		&config))
	{
		ELOG("Error : Failed to create GrassGenerator pipeline");
		return false;
	}

	return true;
	return true;
}

//-----------------------------------------------------------------------------
// 	 マスクドUIパイプラインの初期化
//-----------------------------------------------------------------------------
bool PipelineInitializer::InitalizeMaskedUIPipeline()
{
	if (!ShaderManager::GetInstance().LoadShader(L"MaskedUITexture", L"TextureVS.cso", L"MaskedTexturePS.cso"))
	{
		ELOG("Error : ShaderManager::LoadShader() Failed for MaskedUITexture");
		return false;
	}
	RootSignatureConfig rootSig;
	// b0: SpriteCB（VSで使用）
	rootSig.cbvConfigs.push_back({ ShaderStage::VS, 0, 0 });
	// b1: MaskCB（PSで使用）
	rootSig.cbvConfigs.push_back({ ShaderStage::PS, 1, 1 });
	// t0: Texture（PS）
	rootSig.srvConfigs.push_back({ ShaderStage::PS, 2, 0 });
	// s0: Sampler
	rootSig.samplerConfigs.push_back({ ShaderStage::PS, 0, SamplerState::LinearClamp });
	rootSig.allowIL = true;
	rootSig.allowSO = false;
	PipelineStateDesc desc;
	desc.shaderName = L"MaskedUITexture";
	desc.inputLayout = GetUIInputLayout();
	desc.rtvFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	desc.rasterizerState = DirectX::CommonStates::CullNone;
	desc.blendState = DirectX::CommonStates::NonPremultiplied;
	desc.depthStencilState = DirectX::CommonStates::DepthNone;
	desc.sampleCount = 1;
	desc.sampleQuality = 0;
	desc.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	if (!PipelineStateManager::GetInstance().CreatePipelineState(
		L"MaskedUIPipeline",
		desc,
		&rootSig))
	{
		ELOG("Error : Failed to create MaskedUIPipeline");
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// 	 共通モデル用インプットレイアウトを取得
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetModelInputLayout()
{
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

//-----------------------------------------------------------------------------
// 	 スキ二ングモデル用インプットレイアウトを取得
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetSkinnedModelInputLayout()
{
	return {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(SkinnedMeshVertex, Position),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, (UINT)offsetof(SkinnedMeshVertex, Normal),      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, (UINT)offsetof(SkinnedMeshVertex, TexCoord),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)offsetof(SkinnedMeshVertex, Tangent),     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES",  0, DXGI_FORMAT_R32G32B32A32_UINT,  0, (UINT)offsetof(SkinnedMeshVertex, BoneIndices), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (UINT)offsetof(SkinnedMeshVertex, BoneWeights), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

//-----------------------------------------------------------------------------
// 	 UI用インプットレイアウトを取得
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetUIInputLayout()
{
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

//-----------------------------------------------------------------------------
// 	 地形用インプットレイアウトを取得
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetTerrainInputLayout()
{
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  }
	};
}

//-----------------------------------------------------------------------------
// 	 インスタンシング用インプットレイアウト（メッシュ頂点＋インスタンスWorld）
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetInstancedModelInputLayout()
{
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },

		{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};
}

//-----------------------------------------------------------------------------
// 	 草原描画用ジオメトリシェーダーのインプットレイアウトを取得
//-----------------------------------------------------------------------------
std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineInitializer::GetMeadowGSInputLayout()
{
	return {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "SEED",     0, DXGI_FORMAT_R32_FLOAT,       0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}
