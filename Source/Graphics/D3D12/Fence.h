#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "EncodingUtils/ComPtr.h"
#include <cstdint>

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

	// GPUにシグナルを送信し、フェンス値を返します.
	[[nodiscard]] uint64_t Signal(ID3D12CommandQueue* pQueue);

	// 指定フェンス値にGPUが到達するまで待機
	void WaitForValue(uint64_t value, UINT timeout);

	// GPUが完了済みのフェンス値を取得する
	[[nodiscard]] uint64_t GetCompletedValue() const;

private:
	ComPtr<ID3D12Fence> m_Fence; // フェンスです.
	HANDLE m_Event;              // イベントです.
	UINT64 m_Counter;            // 現在のカウンターです.

	Fence(const Fence&)          = delete; // アクセス禁止.
	void operator=(const Fence&) = delete; // アクセス禁止.
};
