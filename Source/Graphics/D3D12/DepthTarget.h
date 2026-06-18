#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "ComPtr.h"
#include <cstdint>

class DescriptorHandle;
class DescriptorPool;

class DepthTarget
{
public:
	/// <summary>
	/// コンストラクタです.
	/// </summary>
	DepthTarget();

	/// <summary>
	/// デストラクタです
	/// </summary>
	~DepthTarget();

	/// <summary>
	/// 初期化処理を行います.
	/// </summary>
	/// <param name="pDevice"><デバイスです./param>
	/// <param name="pPoolDSV"><ディスクリプタプールです./param>
	/// <param name="width"><横幅です./param>
	/// <param name="height"><縦幅です./param>
	/// <param name="format"><ピクセルフォーマットです./param>
	/// <returns><true 初期化に成功 : false 初期化に失敗./returns>
	bool Init(
		ID3D12Device*   pDevice, 
        DescriptorPool* pPoolDSV,
        DescriptorPool* pPoolSRV,
        uint32_t        width,
        uint32_t        height,
        DXGI_FORMAT     format,
        float           clearDepth,
        uint8_t         clearStencil);

	/// <summary>
	/// 終了処理を行います.
	/// </summary>
	void Term();

	/// <summary>
	/// ディスクリプタハンドル(DSV用)を取得します.
	/// </summary>
	/// <returns><ディスクリプタハンドル(DSV用)を返却します./returns>
	DescriptorHandle* GetHandleDSV() const;

	/// <summary>
	/// ディスクリプタハンドル(SRV用)を取得します.
	/// </summary>
	/// <returns><ディスクリプタハンドル(SRV用)を返却します./returns>
	DescriptorHandle* GetHandleSRV() const;

	/// <summary>
	/// リソースを取得します.
	/// </summary>
	/// <returns><リソースを返却します./returns>
	ID3D12Resource* GetResource() const;

	/// <summary>
	/// リソース設定を取得します.
	/// </summary>
	/// <returns><リソース設定を返却します./returns>
	D3D12_RESOURCE_DESC GetDesc() const;

	/// <summary>
	/// 深度ステンシルビューの設定を取得します.
	/// </summary>
	D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc() const;

	/// <summary>
	/// シェーダリソースビューの設定を取得します.
	/// </summary>
	D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;

	/// <summary>
	/// ビューをクリアします.
	/// </summary>
	void ClearView(ID3D12GraphicsCommandList* pCmdList);

private:

	ComPtr<ID3D12Resource>          m_pTarget;          // リソースです.
    DescriptorHandle*               m_pHandleDSV;       // ディスクリプタハンドル(DSV用)です.
    DescriptorHandle*               m_pHandleSRV;       // ディスクリプタハンドル(SRV用)です.
    DescriptorPool*                 m_pPoolDSV;         // ディスクリプタプール(DSV用)です.
    DescriptorPool*                 m_pPoolSRV;         // ディスクリプタプール(SRV用)です.
    D3D12_DEPTH_STENCIL_VIEW_DESC   m_DSVDesc;          // 深度ステンシルビューの設定です.
    D3D12_SHADER_RESOURCE_VIEW_DESC m_SRVDesc;          // シェーダリソースビューの設定.
    float                           m_ClearDepth;
    uint8_t                         m_ClearStencil;


	DepthTarget		(const DepthTarget&) = delete;
	void operator = (const DepthTarget&) = delete;

};