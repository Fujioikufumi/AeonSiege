#include "LightManager.h"
#include "ResourceManager.h"
#include "Logger.h"
#include "App.h"

//----------------------------------------------------------
// 		初期化処理
//----------------------------------------------------------
bool LightManager::Init()
{
	// リソースマネージャーの取得
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	// DirectX12リソースの取得
	m_pDevice = resourceManager.GetDevice();
	m_pDevice->AddRef();
	m_pPool = resourceManager.GetPool(POOL_TYPE_RES);
	m_pPool->AddRef();

	// ライト定数バッファの初期化(フレームごと)   
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		if (!m_LightCB[i].Init(m_pDevice.Get(), m_pPool, sizeof(CbLight)))
		{
			ELOG("Error : ConstantBuffer::Init() Failed for LightCB[%d]", i);
			Term();
			return false;
		}
	}

	// ライトデフォルト値の設定  
	m_LightData.LightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_LightData.LightIntensity = 1.0f;
	m_LightData.LightForward = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_LightData.AmbientColor = XMFLOAT3(1.0f, 0.2f, 0.2f);
	m_LightData.AmbientIntensity = 0.9f;

	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		Update(i);
	}

	DLOG("LightManager initialized successfully");
	return true;
}

//----------------------------------------------------------
//		終了処理
//----------------------------------------------------------
void LightManager::Term()
{
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		m_LightCB[i].Term();
	}

	if (m_pDevice != nullptr)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	if(m_pPool != nullptr)
	{
		m_pPool->Release();
		m_pPool = nullptr;
	}
}

//----------------------------------------------------------
// 		ライトの色を設定
//----------------------------------------------------------
void LightManager::SetLightColor(const DirectX::XMFLOAT3& color)
{
	m_LightData.LightColor = color;
}

//----------------------------------------------------------
//		ライトの方向を設定 (XMFLOAT3)
//----------------------------------------------------------
void LightManager::SetLightDirection(const DirectX::XMFLOAT3& direction)
{
	m_LightData.LightForward = direction;
}

//----------------------------------------------------------
//		ライトの方向を設定 (XMVECTOR)
//----------------------------------------------------------
void LightManager::SetLightDirection(const XMVECTOR& direction)
{
	XMFLOAT3 dir;
	XMStoreFloat3(&dir, direction);
	m_LightData.LightForward = dir;
}

//----------------------------------------------------------
//		ライトの強度を設定
//----------------------------------------------------------
void LightManager::SetLightIntensity(float intensity)
{
	m_LightData.LightIntensity = intensity;
}

//----------------------------------------------------------
//		環境光の色を設定
//----------------------------------------------------------
void LightManager::SetAmbientColor(const XMFLOAT3& color)
{
	m_LightData.AmbientColor = color;
}

//----------------------------------------------------------
// 		環境光の強度を設定
//----------------------------------------------------------
void LightManager::SetAmbientIntensity(float intensity)
{
	m_LightData.AmbientIntensity = intensity;
}

//----------------------------------------------------------
// 		ライトデータを直接設定
//----------------------------------------------------------
void LightManager::SetLightData(const CbLight& lightData)
{
	m_LightData = lightData;
}

//----------------------------------------------------------
// 		更新処理
//----------------------------------------------------------
void LightManager::Update(uint32_t frameIndex)
{
	if (frameIndex >= m_FrameCount)
	{
		ELOG("Error : LightManager::Update() Invalid frameIndex %d", frameIndex);
		return;
	}

	auto ptr = m_LightCB[frameIndex].GetPtr<CbLight>();
	if (ptr != nullptr)
	{
		
		*ptr = m_LightData;
	}
}

//-----------------------------------------------------------------------------
//		ライト方向ベクトルを取得
//-----------------------------------------------------------------------------
XMVECTOR LightManager::GetLightDirection() const
{
	return XMLoadFloat3(&m_LightData.LightForward);
}
//-----------------------------------------------------------------------------
// 		GPUディスクリプタハンドルを取得
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE LightManager::GetHandleGPU(uint32_t frameIndex) const
{
	if (frameIndex >= m_FrameCount)
	{
		ELOG("Error : Invalid frame index: %d", frameIndex);
		return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
	}

	return m_LightCB[frameIndex].GetHandleGPU();
}