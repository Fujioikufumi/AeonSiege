#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <DescriptorPool.h>
#include <ConstantBuffer.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include "main.h"

class SkyDome
{
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    SkyDome();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~SkyDome();

    /// <summary>
    /// 初期化処理
    /// </summary>
    bool Init(
        ID3D12Device* pDevice,
        DescriptorPool* pPoolRes,
        DXGI_FORMAT     colorFormat,
        DXGI_FORMAT     depthFormat,
        float           radius = 1000.0f,
        int             slices = 32,
        int             stacks = 16);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Term();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw(
        ID3D12GraphicsCommandList* pCmd,
        D3D12_GPU_DESCRIPTOR_HANDLE handleTexture,
        const DirectX::XMMATRIX& viewMatrix,
        const DirectX::XMMATRIX& projMatrix);

private:
    DescriptorPool* m_pPoolRes;                     // ディスクリプタプール
    ComPtr<ID3D12RootSignature>     m_pRootSig;     // ルートシグネチャ
	ComPtr<ID3D12PipelineState>     m_pPSO;         // パイプラインステートオブジェクト
	ConstantBuffer                  m_CB[2];        // 定数バッファ（フレームごと）
    VertexBuffer                    m_VB;           // 頂点バッファ
	IndexBuffer                     m_IB;           // インデックスバッファ
	int                             m_Index;        // 現在のフレームインデックス
	uint32_t                        m_IndexCount;   // インデックス数

	float m_RatationY;   // Y����]�p
};