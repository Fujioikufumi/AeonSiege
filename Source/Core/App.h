#pragma once

#define NOMINMAX

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include "EncodingUtils/ComPtr.h"
#include "Graphics/D3D12/DescriptorPool.h"
#include "Graphics/D3D12/ColorTarget.h"
#include "Graphics/D3D12/DepthTarget.h"
#include "Graphics/D3D12/CommandList.h"
#include "Graphics/D3D12/Fence.h"
#include "Graphics/Model/Mesh.h"
#include "Graphics/D3D12/Texture.h"
#include "Utility/InlineUtil.h"
#include "Core/NameSpace.h"
#include "Core/main.h"

//-----------------------------------------------------------------------------
// Linker
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

///////////////////////////////////////////////////////////////////////////////
// App class
///////////////////////////////////////////////////////////////////////////////
class App
{
	//=========================================================================
	// list of friend classes and methods.
	//=========================================================================
	/* NOTHING */

public:
	//=========================================================================
	// public variables.
	//=========================================================================
	/* NOTHING */

	//=========================================================================
	// public methods.
	//=========================================================================

	//-------------------------------------------------------------------------
	//! @brief      コンストラクタです.
	//-------------------------------------------------------------------------
	App(uint32_t width, uint32_t height, DXGI_FORMAT format);

	//-------------------------------------------------------------------------
	//! @brief      デストラクタです.
	//-------------------------------------------------------------------------
	virtual ~App();

	//-------------------------------------------------------------------------
	//! @brief      アプリケーションを実行します.
	//-------------------------------------------------------------------------
	void Run();

protected:
	///////////////////////////////////////////////////////////////////////////
	// POOL_TYPE enum
	///////////////////////////////////////////////////////////////////////////

	//=========================================================================
	// private variables.
	//=========================================================================
	static const uint32_t FrameCount = static_cast<uint32_t>(::FrameCount); // フレームバッファ数です.

	HINSTANCE m_hInst; // インスタンスハンドルです.
	HWND m_hWnd;       // ウィンドウハンドルです.
	uint32_t m_Width;  // ウィンドウの横幅です.
	uint32_t m_Height; // ウィンドウの縦幅です.

	ComPtr<IDXGIFactory4> m_pFactory;      // DXGIファクトリーです.
	ComPtr<ID3D12Device> m_pDevice;        // デバイスです.
	ComPtr<ID3D12CommandQueue> m_pQueue;   // コマンドキューです.
	ComPtr<IDXGISwapChain4> m_pSwapChain;  // スワップチェインです.
	ColorTarget m_ColorTarget[FrameCount]; // カラーターゲットです.
	DepthTarget m_DepthTarget;             // 深度ターゲットです.
	DescriptorPool* m_pPool[POOL_COUNT];   // ディスクリプタプールです.
	CommandList m_CommandList;             // コマンドリストです.
	Fence m_Fence;                         // フェンスです.
	uint32_t m_FrameIndex;                 // フレーム番号です.
	uint64_t m_FrameFenceValues[FrameCount] = {}; // フェンス値です.
	D3D12_VIEWPORT m_Viewport;             // ビューポートです.
	D3D12_RECT m_Scissor;                  // シザー矩形です.
	DXGI_FORMAT m_BackBufferFormat;        // バックバッファフォーマットです.

	//=========================================================================
	// protected methods.
	//=========================================================================
	uint64_t Present(uint32_t interval);
	[[nodiscard]] uint64_t GetCompletedFenceValue() const { return m_Fence.GetCompletedValue(); }
	bool IsSupportHDR() const;
	float GetMaxLuminance() const;
	float GetMinLuminance() const;
	float GetDeltaTime() const { return m_DeltaTime; }

	virtual bool OnInit()
	{
		return true;
	}

	virtual void OnTerm()
	{ /* DO_NOTHING */
	}

	virtual void OnUpdate()
	{ /* DO_NOTHING */
	}

	virtual void OnRender()
	{ /* DO_NOTHING */
	}

	virtual void OnMsgProc(HWND, UINT, WPARAM, LPARAM)
	{ /* DO_NOTHING */
	}

private:
	//=========================================================================
	// private variables.
	//=========================================================================
	bool m_SupportHDR;    // HDRディスプレイをサポートしているかどうか?
	float m_MaxLuminance; // ディスプレイの最大輝度値.
	float m_MinLuminance; // ディスプレイの裁量輝度値.
	// フレーム時間計算用
	LARGE_INTEGER m_PerformanceFrequency; // パフォーマンスカウンターの周波数
	LARGE_INTEGER m_LastTime;             // 前フレームの時間
	bool m_IsFirstFrame;                  // 初回フレームかどうか
	float m_DeltaTime;                    // 【追加】1フレームの経過時間(秒)

	//=========================================================================
	// private methods.
	//=========================================================================
	bool InitApp();
	void TermApp();
	bool InitWnd();
	void TermWnd();
	bool InitD3D();
	void TermD3D();
	void MainLoop();
	void CheckSupportHDR();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};
