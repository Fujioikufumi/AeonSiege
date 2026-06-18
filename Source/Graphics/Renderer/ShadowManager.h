#pragma once
//======================================================================
// 		Includes
//======================================================================
#include "ConstantBuffer.h"
#include "DepthTarget.h"
#include "NameSpace.h"
#include "ComPtr.h"
#include "GameObject.h"
#include "SingletonManager.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <list>
#include "ModelManager.h"

using namespace DirectX;

struct alignas(256) CbShadow
{
	XMFLOAT4X4 LightView;      // ライトのビュー行列
	XMFLOAT4X4 LightProj;      // ライトのプロジェクション行列
	float ShadowBias;          // シャドウバイアス
	float ShadowMapSize;       // シャドウマップのサイズ
	float PCFKernelSize;       // PCFカーネルサイズ（現在は未使用だが将来の拡張用）
	float Padding;
};

//======================================================================
//		ShadowManagerクラス(シングルトン)
//======================================================================
class ShadowManager : public SingletonManager<ShadowManager>
{
private:
	friend class SingletonManager<ShadowManager>;

	// DirectX12リソース
	ComPtr<ID3D12Device> m_pDevice;
	DescriptorPool* m_pPoolDSV;
	DescriptorPool* m_pPoolSRV;
	DescriptorPool* m_pPoolRES;

	// シャドウマップ
	DepthTarget m_ShadowMap;

	// シャドウデータ
	CbShadow m_ShadowData;

	// フレームカウント
	const uint32_t m_FrameCount = 2;

	// シャドウマップのサイズ
	const uint32_t m_ShadowMapSize = 2048;

	// シャドウ用定数バッファ(フレームカウント分)
	ConstantBuffer m_ShadowCB[2]; // FrameCount = 2

	// シャドウ用Transform定数バッファ(ワールド行列用)
	struct alignas(256) CbShadowTransform
	{
		XMFLOAT4X4 World;
		XMFLOAT4X4 View;
		XMFLOAT4X4 Proj;
	};
	ConstantBuffer m_ShadowTransformCB; // 全オブジェクト共通

	bool m_IsFirstFrame = true;

	// コンストラクタ・デストラクタ・コピー禁止
	ShadowManager() = default;
	~ShadowManager() = default;
	ShadowManager(const ShadowManager&) = delete;
	void operator=(const ShadowManager&) = delete;

public:
	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	//==================================================================
	// 			影生成処理
	//==================================================================

	/// <summary>
	/// ライトのビュー行列・プロジェクション行列・ビュー×プロジェクション行列の計算
	/// </summary>
	void CalculateLightMatrices(
		const XMFLOAT3& lightDirection,
		XMMATRIX& outLightView,
		XMMATRIX& outLightProj,
		XMMATRIX& outLightViewProj);

	/// <summary>
	/// シャドウマップのレンダリング
	/// </summary>
	void RenderShadowMap(
		ID3D12GraphicsCommandList* pCmdList,
		const XMFLOAT3& lightDirection,
		const std::list<GameObject*>& shadowCasters);

	/// <summary>
	/// シャドウマップ用定数バッファの更新
	/// </summary>
	void Update(uint32_t frameIndex, const XMFLOAT3& lightDirection);

	//==================================================================
	//		�Q�b�^�[
	//==================================================================

	/// <summary>
	/// GPUディスクリプタハンドルの取得
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU(uint32_t frameIndex) const;

	/// <summary>
	/// GPUディスクリプタハンドルの取得(シャドウマップSRV)
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetShadowMapSRV() const;

	/// <summary>
	/// シャドウマップの取得
	/// </summary>
	DepthTarget* GetShadowMap() { return &m_ShadowMap; }

	/// <summary>
	///　シャドウマップのサイズの取得
	/// </summary>
	uint32_t GetShadowMapSize() const { return m_ShadowMapSize; }

	/// <summary>
	///　シャドウバイアスの設定
	/// </summary>
	void SetShadowBias(float bias) { m_ShadowData.ShadowBias = bias; }

	/// <summary>
	///　PCFカーネルサイズの設定
	/// </summary>
	void SetPCFKernelSize(float kernelSize) { m_ShadowData.PCFKernelSize = kernelSize; }
};