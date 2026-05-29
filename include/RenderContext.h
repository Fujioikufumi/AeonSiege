#pragma once
//======================================================================
// RenderContext
//======================================================================
#include <d3d12.h>
#include <DirectXMath.h>

struct RenderContext
{
    ID3D12GraphicsCommandList* pCmdList;        // コマンドリスト
    D3D12_GPU_DESCRIPTOR_HANDLE transformCB;    // Transform定数バッファ
    D3D12_GPU_DESCRIPTOR_HANDLE lightCB;        // Light定数バッファ
    D3D12_GPU_DESCRIPTOR_HANDLE cameraCB;       // Camera定数バッファ
    D3D12_GPU_DESCRIPTOR_HANDLE meshCB;         // メッシュ用定数バッファ
    D3D12_GPU_DESCRIPTOR_HANDLE shadowCB;       // シャドウ定数バッファ
    D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRV;   // シャドウマップSRV
    DirectX::XMMATRIX viewMatrix;               // ビュー行列
    DirectX::XMMATRIX projMatrix;               // プロジェクション行列
	DirectX::XMMATRIX viewProjMatrix;           // ビュー・プロジェクション行列
	DirectX::XMFLOAT3 cameraPos;			    // カメラ位置
    D3D12_VIEWPORT viewport;                    // ビューポート
    D3D12_RECT scissorRect;                     // シザー矩形
};
