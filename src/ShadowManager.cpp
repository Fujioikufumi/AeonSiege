#include "ShadowManager.h"
#include "ResourceManager.h"
#include "Logger.h"
#include "RenderSystem.h"
#include "PipelineStateManager.h"
#include "ShaderManager.h"
#include "RenderContext.h"
#include <DirectXMath.h>
#include "MeshRenderer.h"
#include "DirectXHelpers.h"
#include "Texture.h"

using namespace DirectX;

//----------------------------------------------------------
// 		初期化処理
//----------------------------------------------------------
bool ShadowManager::Init()
{
	Term();

	// リソースマネージャーのインスタンスを取得
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	// デバイスとプールの取得
	m_pDevice = resourceManager.GetDevice();
	if (m_pDevice == nullptr)
	{
		ELOG("Error : Failed to get device from ResourceManager");
		return false;
	}
	m_pDevice->AddRef();

	m_pPoolDSV = resourceManager.GetPool(POOL_TYPE_DSV);
	if (m_pPoolDSV == nullptr)
	{
		ELOG("Error : Failed to get DSV pool from ResourceManager");
		return false;
	}
	m_pPoolDSV->AddRef();

	m_pPoolSRV = resourceManager.GetPool(POOL_TYPE_RES);
	if (m_pPoolSRV == nullptr)
	{
		ELOG("Error : Failed to get SRV pool from ResourceManager");
		return false;
	}
	m_pPoolSRV->AddRef();

	m_pPoolRES = resourceManager.GetPool(POOL_TYPE_RES);
	if (m_pPoolRES == nullptr)
	{
		ELOG("Error : Failed to get RES pool from ResourceManager");
		return false;
	}
	m_pPoolRES->AddRef();

	// シャドウマップの初期化
	if (!m_ShadowMap.Init(
		m_pDevice.Get(),
		m_pPoolDSV,
		m_pPoolSRV,
		m_ShadowMapSize,
		m_ShadowMapSize,
		DXGI_FORMAT_D32_FLOAT,
		1.0f,
		0))
	{
		ELOG("Error : ShadowMap::Init() Failed");
		Term();
		return false;
	}

	// シャドウ用定数バッファの初期化
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		if (!m_ShadowCB[i].Init(m_pDevice.Get(), m_pPoolRES, sizeof(CbShadow)))
		{
			ELOG("Error : ConstantBuffer::Init() Failed for ShadowCB[%d]", i);
			Term();
			return false;
		}
	}

	// シャドウ用Transform定数バッファの初期化
	if (!m_ShadowTransformCB.Init(m_pDevice.Get(), m_pPoolRES, sizeof(CbShadowTransform)))
	{
		ELOG("Error : ConstantBuffer::Init() Failed for ShadowTransformCB");
		Term();
		return false;
	}

	// シャドウデータの初期化
	m_ShadowData.ShadowBias = 0.0005f;
	m_ShadowData.ShadowMapSize = static_cast<float>(m_ShadowMapSize);
	m_ShadowData.PCFKernelSize = 3.0f; // 3x3 PCF

	// 初期状態のシャドウバッファを更新
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		Update(i, XMFLOAT3(0.0f, -1.0f, 0.0f));
	}

	m_IsFirstFrame = true;

	DLOG("ShadowManager initialized successfully");

	return true;
}

//----------------------------------------------------------
//		終了処理
//----------------------------------------------------------
void ShadowManager::Term()
{
	// シャドウマップの終了処理
	m_ShadowMap.Term();

	m_ShadowTransformCB.Term();

	//	シャドウ定数バッファの終了処理
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		m_ShadowCB[i].Term();
	}

	// デバイスの解放
	if (m_pDevice != nullptr)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	// プールの解放
	if (m_pPoolDSV != nullptr)
	{
		m_pPoolDSV->Release();
		m_pPoolDSV = nullptr;
	}

	if (m_pPoolSRV != nullptr)
	{
		m_pPoolSRV->Release();
		m_pPoolSRV = nullptr;
	}

	if (m_pPoolRES != nullptr)
	{
		m_pPoolRES->Release();
		m_pPoolRES = nullptr;
	}
}

//----------------------------------------------------------
// 		ライトのビュー/プロジェクション行列を計算
//----------------------------------------------------------
void ShadowManager::CalculateLightMatrices(
	const XMFLOAT3& lightDirection,
	XMMATRIX& outLightView,
	XMMATRIX& outLightProj,
	XMMATRIX& outLightViewProj)
{
	// ライト方向ベクトルを正規化
	XMVECTOR lightDirVec = XMLoadFloat3(&lightDirection);
	lightDirVec = XMVector3Normalize(lightDirVec);

	// シーンの中心位置（原点）からライト方向に一定距離離れた位置にライトを配置
	XMVECTOR sceneCenter = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	float lightDistance = 50.0f;  // ライトまでの距離
	XMVECTOR lightPos = XMVectorSubtract(sceneCenter, XMVectorScale(lightDirVec, lightDistance));

	// DirectX11と同じ方法でライトのビュー行列を計算
	// ライト位置から、ライト位置+ライト方向を見る
	XMVECTOR lookAt = XMVectorAdd(lightPos, lightDirVec);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	outLightView = XMMatrixLookAtLH(lightPos, lookAt, up);

	// DirectX11と同じ直交投影パラメータを使用
	float shadowMapSize = 120.0f;  // Planeのサイズ（100）より大きめに設定
	outLightProj = XMMatrixOrthographicLH(shadowMapSize, shadowMapSize, 0.1f, 200.0f);


	// DirectX11では転置しているが、DirectX12では通常転置不要
	// ただし、シェーダーでの乗算順序に合わせる必要がある
	outLightViewProj = outLightView * outLightProj;
}
//----------------------------------------------------------
//		シャドウマップ生成
//----------------------------------------------------------
void ShadowManager::RenderShadowMap(
	ID3D12GraphicsCommandList* pCmdList,
	const XMFLOAT3& lightDirection,
	const std::list<GameObject*>& shadowCasters)
{
	// ライト行列を計算
	XMMATRIX lightView, lightProj, lightViewProj;
	CalculateLightMatrices(lightDirection, lightView, lightProj, lightViewProj);

	// リソースステート遷移
	auto* shadowMapResource = m_ShadowMap.GetResource();

	if (shadowMapResource != nullptr)
	{
		if (m_IsFirstFrame)
		{
			// 最初のフレーム：既にDEPTH_WRITE状態なので遷移不要
			m_IsFirstFrame = false;
		}
		else
		{
			// 2フレーム目以降：PIXEL_SHADER_RESOURCEからDEPTH_WRITEに遷移
			DirectX::TransitionResource(pCmdList,
				shadowMapResource,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
	}

	// シャドウマップにレンダーターゲットを設定
	auto shadowDSV = m_ShadowMap.GetHandleDSV();
	if (shadowDSV == nullptr)
	{
		ELOG("Error : ShadowMap DSV handle is null");
		return;
	}

	pCmdList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV->HandleCPU);

	// ビューポートとシザーを設定
	D3D12_VIEWPORT shadowViewport = {
		0.0f, 0.0f,
		static_cast<float>(m_ShadowMapSize),
		static_cast<float>(m_ShadowMapSize),
		0.0f, 1.0f
	};
	D3D12_RECT shadowScissor = { 0, 0, static_cast<LONG>(m_ShadowMapSize), static_cast<LONG>(m_ShadowMapSize) };
	pCmdList->RSSetViewports(1, &shadowViewport);
	pCmdList->RSSetScissorRects(1, &shadowScissor);

	// シャドウマップをクリア
	m_ShadowMap.ClearView(pCmdList);

	// シャドウパイプラインを設定
	auto* shadowPipeline = PipelineStateManager::GetInstance().GetPipelineState(L"ShadowPipeline");
	pCmdList->SetPipelineState(shadowPipeline->pPSO.Get());
	pCmdList->SetGraphicsRootSignature(shadowPipeline->rootSig.GetPtr());

	if (!shadowPipeline || !shadowPipeline->isValid)
	{
		ELOG("Warning : ShadowPipeline not found. Shadow map rendering skipped.");
		return;
	}

	// 各オブジェクトを描画
	for (GameObject* pObj : shadowCasters)
	{
		if (pObj == nullptr) continue;

		// 描画するオブジェクトのMeshRendererコンポーネントを取得
		MeshRenderer* pMeshRenderer = pObj->GetComponent<MeshRenderer>();
		// なければスキップ
		if (pMeshRenderer == nullptr) continue;

		// モデルパスを取得
		std::wstring modelPath = pMeshRenderer->GetModelPath();
		// モデルパスからモデル情報を取得
		ModelInfo* pModelInfo = ModelManager::GetInstance().GetModel(modelPath);
		if (pModelInfo == nullptr) continue;

		// ワールド行列を取得
		XMMATRIX worldMatrix = pObj->GetWorldMatrix();

		// Transform定数バッファを更新（ライト視点のView/Projを使用）
		auto ptr = m_ShadowTransformCB.GetPtr<CbShadowTransform>();
		if (ptr != nullptr)
		{
			// DirectX11に合わせる：行列を転置してから書き込む
			XMMATRIX transposedWorld = XMMatrixTranspose(worldMatrix);  // World行列も転置
			XMMATRIX transposedView = XMMatrixTranspose(lightView);
			XMMATRIX transposedProj = XMMatrixTranspose(lightProj);

			XMStoreFloat4x4(&ptr->World, transposedWorld);  // 転置したWorldを書き込む
			XMStoreFloat4x4(&ptr->View, transposedView);
			XMStoreFloat4x4(&ptr->Proj, transposedProj);
		}

		// Transform定数バッファをルートシグネチャに設定（register b0）
		D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle = pMeshRenderer->GetMeshCBHandle();
		pCmdList->SetGraphicsRootDescriptorTable(1, meshCBHandle); // register(b1) に World 行列

		// モデル内の各メッシュを描画
		for (size_t i = 0; i < pModelInfo->meshes.size(); ++i)
		{
			Mesh* pMesh = pModelInfo->meshes[i];
			if (pMesh == nullptr) continue;

			pMesh->Draw(pCmdList);
		}
	}

	// シャドウマップのリソースステートをDEPTH_WRITEからSHADER_RESOURCEに遷移
	if (shadowMapResource != nullptr)
	{
		DirectX::TransitionResource(pCmdList,
			shadowMapResource,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

//----------------------------------------------------------
//		シャドウマップ用定数バッファの更新
//----------------------------------------------------------
void ShadowManager::Update(uint32_t frameIndex, const XMFLOAT3& lightDirection)
{
	if (frameIndex >= m_FrameCount)
	{
		ELOG("Error : ShadowManager::Update() Invalid frameIndex %d", frameIndex);
		return;
	}

	// ライト行列を計算
	XMMATRIX lightView, lightProj, lightViewProj;
	CalculateLightMatrices(lightDirection, lightView, lightProj, lightViewProj);

	// DirectX11と同じ：行列を転置してから定数バッファに書き込む
	lightView = XMMatrixTranspose(lightView);
	lightProj = XMMatrixTranspose(lightProj);

	// 定数バッファに書き込む（DirectX11の構造に合わせる）
	auto ptr = m_ShadowCB[frameIndex].GetPtr<CbShadow>();
	if (ptr != nullptr)
	{
		XMStoreFloat4x4(&ptr->LightView, lightView);
		XMStoreFloat4x4(&ptr->LightProj, lightProj);
		ptr->ShadowBias = m_ShadowData.ShadowBias;
		ptr->ShadowMapSize = m_ShadowData.ShadowMapSize;
		ptr->PCFKernelSize = m_ShadowData.PCFKernelSize;
	}
}

//-----------------------------------------------------------------------------
//		GPUハンドルを取得
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE ShadowManager::GetHandleGPU(uint32_t frameIndex) const
{
	if (frameIndex >= m_FrameCount)
	{
		ELOG("Error : Invalid frame index: %d", frameIndex);
		return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
	}

	return m_ShadowCB[frameIndex].GetHandleGPU();
}

//-----------------------------------------------------------------------------
// 		シャドウマップSRVのGPUハンドルを取得
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE ShadowManager::GetShadowMapSRV() const
{
	auto handle = m_ShadowMap.GetHandleSRV();
	if (handle != nullptr)
	{
		return handle->HandleGPU;
	}
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}