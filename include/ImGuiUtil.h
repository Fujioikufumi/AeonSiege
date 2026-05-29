#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <ComPtr.h>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"
#include <cstdint>

class ImGuiUtil
{
public:
	ImGuiUtil();
	virtual ~ImGuiUtil();

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init(HWND hWnd, ID3D12Device* pDevice, uint32_t frameCount, DXGI_FORMAT rtvFormat);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// 新規フレーム処理
	/// </summary>
	void NewFrame();

	/// <summary>
	/// レンダリング処理
	/// </summary>
	void Render(ID3D12GraphicsCommandList* pCmdList);

private:
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	bool m_Initalized = false;
};