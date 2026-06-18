#pragma once

//----------------------------------------------------------------------
// 		Includes
//----------------------------------------------------------------------
#include "ConstantBuffer.h"
#include "NameSpace.h"
#include "ComPtr.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "SingletonManager.h"

using namespace DirectX;

//----------------------------------------------------------------------
//		LightManagerクラス(シングルトン)
//----------------------------------------------------------------------
class LightManager : public SingletonManager<LightManager>
{
private:
	friend class SingletonManager<LightManager>;

	// DirectX12リソース
	ComPtr<ID3D12Device> m_pDevice;
	DescriptorPool* m_pPool;

	// ライト定数バッファ(フレームごと)
	ConstantBuffer m_LightCB[2]; // FrameCount = 2

	// ライトデータ
	CbLight m_LightData;

	// フレームカウント
	const uint32_t m_FrameCount = 2;

	// コンストラクタ・デストラクタ (シングルトン)
	LightManager() = default;
	~LightManager() = default;
	LightManager(const LightManager&) = delete;
	void operator=(const LightManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	bool Init();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();


	/// <summary>
	/// ライトの色を設定
	/// </summary>
	/// <param name="color"></param>
	void SetLightColor(const DirectX::XMFLOAT3& color);

	/// <summary>
	/// ライトの方向を設定(XMFLOAT3)
	/// </summary>
	void SetLightDirection(const DirectX::XMFLOAT3& direction);

	/// <summary>
	/// ライトの方向を設定(XMVECTOR)
	/// </summary>
	void SetLightDirection(const XMVECTOR& direction);

	/// <summary>
	/// ライトの強度を設定
	/// </summary>
	void SetLightIntensity(float intensity);

	/// <summary>
	/// 環境光の色を設定
	/// </summary>
	void SetAmbientColor(const XMFLOAT3& color);

	/// <summary>
	/// 環境光の強度を設定
	/// </summary>
	void SetAmbientIntensity(float intensity);

	/// <summary>
	/// ライトデータを直接設定
	/// </summary>
	void SetLightData(const CbLight& lightData);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(uint32_t frameIndex);

	/// <summary>
	/// ライト方向ベクトルを取得
	/// </summary>
	XMVECTOR GetLightDirection() const;

	XMFLOAT3 GetAmbientColor() { return m_LightData.AmbientColor; }

	float GetAmbientIntensity() { return m_LightData.AmbientIntensity; }

	/// <summary>
	/// GPUディスクリプタハンドルを取得
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU(uint32_t frameIndex) const;
};