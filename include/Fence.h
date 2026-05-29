#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "ComPtr.h"

class Fence
{
public:
	Fence();
	~Fence();
	bool Init(ID3D12Device* pDevice);
	void Term();

	/// <summary>
	/// シグナル状態になるまで指定された時間待機します.
	/// </summary>
	/// <param name="pQueue"><コマンドキューです./param>
	/// <param name="timeout"><タイムアウト時間(ミリ秒)./param>
	void Wait(ID3D12CommandQueue* pQueue, UINT timeout);

	/// <summary>
	/// シグナル状態になるまでずっと待機します.
	/// </summary>
	/// <param name="pQueue"><コマンドキューです./param>
	void Sync(ID3D12CommandQueue* pQueue);

private:
	ComPtr<ID3D12Fence> m_Fence;           // フェンスです.
	HANDLE              m_Event;            // イベントです.
	UINT                m_Counter;          // 現在のカウンターです.

	Fence			(const Fence&) = delete;    // アクセス禁止.
	void operator = (const Fence&) = delete;    // アクセス禁止.
};