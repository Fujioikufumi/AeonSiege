#include "PipelineStateManager.h"
#include "Logger.h"
#include "CommonStates.h"
#include "ResourceManager.h"
#include "ShaderManager.h"

//----------------------------------------------------------------------
/// 初期化処理
//----------------------------------------------------------------------
bool PipelineStateManager::Init()
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	m_Device = resourceManager.GetDevice();
	m_Pool = resourceManager.GetPool(POOL_TYPE_RES);

	// エラーチェック
	if (!m_Device || !m_Pool)
	{
		ELOG("Error : ResourceManager not initialized");
		return false;
	}

	return true;
}

//----------------------------------------------------------------------
/// モデル用のデフォルトルートシグネチャ設定
//----------------------------------------------------------------------
RootSignatureConfig PipelineStateManager::CreateDefaultModelRootSignatureConfig()
{
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

	return config;
}

//----------------------------------------------------------------------
/// ルートシグネチャを作成
//----------------------------------------------------------------------
bool PipelineStateManager::CreateRootSignature(const RootSignatureConfig& config, RootSignature& rootSig)
{
	// パラメータ数を計算（最大インデックス+1）
	int maxIndex = 0;
	for (const auto& cbv : config.cbvConfigs)
	{
		if (cbv.index > maxIndex)
			maxIndex = cbv.index;
	}
	for (const auto& srv : config.srvConfigs)
	{
		if (srv.index > maxIndex)
			maxIndex = srv.index;
	}
	for(const auto& uav : config.uavConfigs)
	{
		if (uav.index > maxIndex)
			maxIndex = uav.index;
	}
	int paramCount = maxIndex + 1;

	RootSignature::Desc desc;
	desc.Begin(paramCount);

	// CBV設定
	for (const auto& cbv : config.cbvConfigs)
	{
		desc.SetCBV(cbv.stage, cbv.index, cbv.reg);
	}

	// SRV設定
	for (const auto& srv : config.srvConfigs)
	{
		desc.SetSRV(srv.stage, srv.index, srv.reg);
	}

	// UAV設定
	for (const auto& uav : config.uavConfigs)
	{
		desc.SetUAV(uav.stage, uav.index, uav.reg);
	}

	// サンプラーを設定
	for (const auto& sampler : config.samplerConfigs)
	{
		desc.AddStaticSmp(sampler.stage, sampler.reg, sampler.state);
	}

	// シャドウサンプラーを設定
	for (const auto& shadowSmp : config.shadowSamplerConfigs)
	{
		desc.AddShadowSampler(shadowSmp.stage, shadowSmp.reg);
	}

	// フラグを設定
	if (config.allowIL)
	{
		desc.AllowIL();
	}
	if (config.allowSO)
	{
		desc.AllowSO();
	}

	desc.End();

	// ルートシグネチャを初期化
	if (!rootSig.Init(m_Device.Get(), desc.GetDesc()))
	{
		ELOG("Error : RootSignature::Init() Failed");
		return false;
	}

	return true;

}


//----------------------------------------------------------------------
/// パイプラインステートを作成（簡易版）
//----------------------------------------------------------------------
bool PipelineStateManager::CreatePipelineState(
	const std::wstring& pipelineName,
	const std::wstring& shaderName,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
	DXGI_FORMAT rtvFormat,
	DXGI_FORMAT dsvFormat,
	const RootSignatureConfig* rootSigConfig)
{
	// 既に作成されているか確認
	if (IsPipelineStateCreated(pipelineName))
	{
		DLOG("Pipeline state already created : %ls", pipelineName.c_str())
			return true;
	}

	// シェーダーを取得
	ShaderInfo* shader = ShaderManager::GetInstance().GetShader(shaderName);
	if (shader == nullptr)
	{
		ELOG("Error : Shader not found. shaderName = %ls", shaderName.c_str());
		return false;
	}

	// pVSBlobとpPSBlobがnullptrでないかチェック
	if (shader->pVSBlob.Get() == nullptr)
	{
		ELOG("Error : Vertex shader blob is null. shaderName = %ls, vsPath = %ls",
			shaderName.c_str(), shader->vsPath.c_str());
		return false;
	}

	if (shader->pPSBlob.Get() == nullptr)
	{
		ELOG("Error : Pixel shader blob is null. shaderName = %ls, psPath = %ls",
			shaderName.c_str(), shader->psPath.c_str());
		return false;
	}

	// ルートシグネチャ設定（指定がない場合はデフォルト）
	RootSignatureConfig config = (rootSigConfig != nullptr)
		? *rootSigConfig
		: CreateDefaultModelRootSignatureConfig();


	// パイプラインステート情報を作成
	PipelineStateInfo pipelineInfo;
	pipelineInfo.shaderName = shaderName;
	pipelineInfo.pipelineName = pipelineName;
	pipelineInfo.isValid = false;

	// ルートシグネチャを作成
	if (!CreateRootSignature(config, pipelineInfo.rootSig))
	{
		ELOG("Error : Failed to create root signature. pipelineName = %ls", pipelineName.c_str());
		return false;
	}

	// パイプラインステートを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
	desc.pRootSignature = pipelineInfo.rootSig.GetPtr();
	desc.VS = { shader->pVSBlob->GetBufferPointer(), shader->pVSBlob->GetBufferSize() };
	desc.PS = { shader->pPSBlob->GetBufferPointer(), shader->pPSBlob->GetBufferSize() };
	if (shader->pGSBlob.Get() != nullptr)
	{
		desc.GS = { shader->pGSBlob->GetBufferPointer(), shader->pGSBlob->GetBufferSize() };
	}
	desc.RasterizerState = DirectX::CommonStates::CullNone;
	desc.BlendState = DirectX::CommonStates::AlphaBlend;
	desc.DepthStencilState = DirectX::CommonStates::DepthDefault;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RTVFormatがUNKNOWNの場合はNumRenderTargetsを0にする
	if (rtvFormat == DXGI_FORMAT_UNKNOWN)
	{
		desc.NumRenderTargets = 0;
		desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	}
	else
	{
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = rtvFormat;
	}

	desc.DSVFormat = dsvFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	HRESULT hr = m_Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineInfo.pPSO.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. pipelineName = %ls, hr = 0x%x",
			pipelineName.c_str(), hr);
		pipelineInfo.rootSig.Term();
		return false;
	}

	pipelineInfo.isValid = true;
	m_PipelineStates[pipelineName] = std::move(pipelineInfo);

	DLOG("Pipeline state created successfully : %ls", pipelineName.c_str());
	return true;
}

//======================================================================
//		パイプラインステートを作成 (詳細版)
//======================================================================
bool PipelineStateManager::CreatePipelineState(
	const std::wstring& pipelineName,
	const PipelineStateDesc& desc,
	const RootSignatureConfig* rootSigConfig)
{
	// 既に作成されているか確認
	if (IsPipelineStateCreated(pipelineName))
	{
		DLOG("Pipeline state already created: %ls", pipelineName.c_str());
		return true;
	}

	// シェーダーを取得
	ShaderInfo* shader = ShaderManager::GetInstance().GetShader(desc.shaderName);
	if (shader == nullptr)
	{
		ELOG("Error : Shader not found. shaderName = %ls", desc.shaderName.c_str());
		return false;
	}

	// ルートシグネチャ設定（指定がない場合はデフォルト）
	RootSignatureConfig config = (rootSigConfig != nullptr)
		? *rootSigConfig
		: CreateDefaultModelRootSignatureConfig();

	// パイプラインステート情報を作成
	PipelineStateInfo pipelineInfo;
	pipelineInfo.shaderName = desc.shaderName;
	pipelineInfo.pipelineName = pipelineName;
	pipelineInfo.isValid = false;

	// ルートシグネチャを作成
	if (!CreateRootSignature(config, pipelineInfo.rootSig))
	{
		ELOG("Error : Failed to create root signature. pipelineName = %ls", pipelineName.c_str());
		return false;
	}

	// パイプラインステートを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { desc.inputLayout.data(), static_cast<UINT>(desc.inputLayout.size()) };
	psoDesc.pRootSignature = pipelineInfo.rootSig.GetPtr();
	psoDesc.VS = { shader->pVSBlob->GetBufferPointer(), shader->pVSBlob->GetBufferSize() };
	psoDesc.PS = { shader->pPSBlob->GetBufferPointer(), shader->pPSBlob->GetBufferSize() };
	if (shader->pGSBlob.Get() != nullptr)
	{
		psoDesc.GS = { shader->pGSBlob->GetBufferPointer(), shader->pGSBlob->GetBufferSize() };
	}
	
	psoDesc.RasterizerState = desc.rasterizerState;
	psoDesc.BlendState = desc.blendState;
	psoDesc.DepthStencilState = desc.depthStencilState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = desc.primitiveTopology;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = desc.rtvFormat;
	psoDesc.DSVFormat = desc.dsvFormat;
	psoDesc.SampleDesc.Count = desc.sampleCount;
	psoDesc.SampleDesc.Quality = desc.sampleQuality;

	HRESULT hr = m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineInfo.pPSO.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. pipelineName = %ls, hr = 0x%x",
			pipelineName.c_str(), hr);
		pipelineInfo.rootSig.Term();
		return false;
	}

	pipelineInfo.isValid = true;
	m_PipelineStates[pipelineName] = std::move(pipelineInfo);

	DLOG("Pipeline state created successfully: %ls", pipelineName.c_str());
	return true;
}

//----------------------------------------------------------------------
//  コンピュートパイプラインステートを作成
//----------------------------------------------------------------------
bool PipelineStateManager::CreateComputePipelineState(const std::wstring& pipelineName, const std::wstring& computeShaderName, const RootSignatureConfig* rootSigConfig)
{
	// 既に作成されているか確認
	if(IsPipelineStateCreated(pipelineName))
	{
		return true;
	}

	// 作成済みのシェーダーを取得
	ShaderInfo* pCsInfo = ShaderManager::GetInstance().GetComputeShader(computeShaderName);
	if (pCsInfo == nullptr || pCsInfo->pCSBlob.Get() == nullptr)
	{
		ELOG("Error : Compute shader not found or invalid. name = %ls", computeShaderName.c_str());
		return false;
	}

	if (rootSigConfig == nullptr)
	{
		ELOG("Error : CreateComputePipelineState requires rootSigConfig");
		return false;
	}

	// パイプライン構造体を作成
	PipelineStateInfo pipelineInfo;
	pipelineInfo.shaderName = computeShaderName;
	pipelineInfo.pipelineName = pipelineName;
	pipelineInfo.isValid = false;

	// ルートシグネチャを作成
	if (!CreateRootSignature(*rootSigConfig, pipelineInfo.rootSig))
	{
		ELOG("Error : Failed to create root signature. pipelineName = %ls", pipelineName.c_str());
		return false;
	}

	// コンピュートパイプラインステートを作成
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = pipelineInfo.rootSig.GetPtr();
	desc.CS.pShaderBytecode = pCsInfo->pCSBlob->GetBufferPointer();
	desc.CS.BytecodeLength = pCsInfo->pCSBlob->GetBufferSize();

	HRESULT hr = m_Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(pipelineInfo.pPSO.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : CreateComputePipelineState Failed. pipelineName = %ls, hr = 0x%x", pipelineName.c_str(), hr);
		pipelineInfo.rootSig.Term();
		return false;
	}

	pipelineInfo.isValid = true;
	m_PipelineStates[pipelineName] = std::move(pipelineInfo);
	DLOG("Compute pipeline created: %ls", pipelineName.c_str());

	return true;
}

//----------------------------------------------------------------------
/// パイプラインステート情報を取得
//----------------------------------------------------------------------
PipelineStateInfo* PipelineStateManager::GetPipelineState(const std::wstring& pipelineName)
{
	auto it = m_PipelineStates.find(pipelineName);
	if (it != m_PipelineStates.end() && it->second.isValid)
	{
		return &it->second;
	}
	return nullptr;
}

//----------------------------------------------------------------------
/// パイプラインステートが作成されているか確認
//----------------------------------------------------------------------
bool PipelineStateManager::IsPipelineStateCreated(const std::wstring& pipelineName) const
{
	auto it = m_PipelineStates.find(pipelineName);
	return (it != m_PipelineStates.end() && it->second.isValid);
}

//----------------------------------------------------------------------
/// パイプラインステートを削除
//----------------------------------------------------------------------
void PipelineStateManager::RemovePipelineState(const std::wstring& pipelineName)
{
	auto it = m_PipelineStates.find(pipelineName);
	if (it != m_PipelineStates.end())
	{
		it->second.pPSO.Reset();
		it->second.rootSig.Term();
		m_PipelineStates.erase(it);
		DLOG("Pipeline state removed: %ls", pipelineName.c_str());
	}
}

//----------------------------------------------------------------------
/// 全てのパイプラインステートを削除
//----------------------------------------------------------------------
void PipelineStateManager::RemoveAll()
{
	for (auto& pair : m_PipelineStates)
	{
		pair.second.pPSO.Reset();
		pair.second.rootSig.Term();
	}
	m_PipelineStates.clear();
	DLOG("All pipeline states removed");
}

//----------------------------------------------------------------------
/// 終了処理
//----------------------------------------------------------------------
void PipelineStateManager::Term()
{
	RemoveAll();
	m_Device.Reset();
	m_Pool = nullptr;
}