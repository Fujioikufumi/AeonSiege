//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Graphics/D3D12/Fence.h"

///////////////////////////////////////////////////////////////////////////////
// Fence class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Fence::Fence()
    : m_Fence(nullptr), m_Event(nullptr), m_Counter(0)
{ /* DO_NOTHING */
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Fence::~Fence()
{
	Term();
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Fence::Init(ID3D12Device* pDevice)
{
	if (pDevice == nullptr)
	{
		return false;
	}

	// イベントを生成.
	m_Event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_Event == nullptr)
	{
		return false;
	}

	// フェンスを生成.
	auto hr = pDevice->CreateFence(
	    0,
	    D3D12_FENCE_FLAG_NONE,
	    IID_PPV_ARGS(m_Fence.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// カウンタ設定.
	m_Counter = 1;

	// 正常終了.
	return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Fence::Term()
{
	// ハンドルを閉じる.
	if (m_Event != nullptr)
	{
		CloseHandle(m_Event);
		m_Event = nullptr;
	}

	// フェンスオブジェクトを破棄.
	m_Fence.Reset();

	// カウンターリセット.
	m_Counter = 0;
}

//-----------------------------------------------------------------------------
//      シグナル状態になるまで指定時間待機します.
//-----------------------------------------------------------------------------
void Fence::Wait(ID3D12CommandQueue* pQueue, UINT timeout)
{
	const auto fenceValue = m_Counter;

	// シグナル処理.
	auto hr = pQueue->Signal(m_Fence.Get(), fenceValue);
	if (FAILED(hr))
	{
		return;
	}

	// カウンターを増やす.
	m_Counter++;

	// 次のフレームの描画準備がまだであれば待機する.
	if (m_Fence->GetCompletedValue() < fenceValue)
	{
		// 完了時にイベントを設定.
		auto hr = m_Fence->SetEventOnCompletion(fenceValue, m_Event);
		if (FAILED(hr))
		{
			return;
		}

		// 待機処理.
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, timeout, FALSE))
		{
			return;
		}
	}
}

//-----------------------------------------------------------------------------
//      シグナル状態になるまでずっと待機します.
//-----------------------------------------------------------------------------
void Fence::Sync(ID3D12CommandQueue* pQueue)
{
	if (pQueue == nullptr)
	{
		return;
	}

	// シグナル処理.
	auto hr = pQueue->Signal(m_Fence.Get(), m_Counter);
	if (FAILED(hr))
	{
		return;
	}

	// 完了時にイベントを設定.
	hr = m_Fence->SetEventOnCompletion(m_Counter, m_Event);
	if (FAILED(hr))
	{
		return;
	}

	// 待機処理.
	if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_Event, INFINITE, FALSE))
	{
		return;
	}

	// カウンターを増やす.
	m_Counter++;
}

//-----------------------------------------------------------------------------
//      シグナルだけ積み, フェンス値を返します(待機しない).
//-----------------------------------------------------------------------------
uint64_t Fence::Signal(ID3D12CommandQueue* pQueue)
{
	const uint64_t fenceValue = m_Counter;

	if (FAILED(pQueue->Signal(m_Fence.Get(), fenceValue)))
	{
		return fenceValue;
	}

	m_Counter++;
	return fenceValue;
}

//-----------------------------------------------------------------------------
//      指定フェンス値にGPUが到達するまで待機します.
//-----------------------------------------------------------------------------
void Fence::WaitForValue(uint64_t value, UINT timeout)
{
	// 既に到達済みなら待たない(起動直後の値0でも安全にスルー)
	if (m_Fence->GetCompletedValue() >= value)
	{
		return;
	}

	if (FAILED(m_Fence->SetEventOnCompletion(value, m_Event)))
	{
		return;
	}

	WaitForSingleObjectEx(m_Event, timeout, FALSE);
}

//-----------------------------------------------------------------------------
//	  GPUが完了済みのフェンス値を取得します.
//-----------------------------------------------------------------------------
uint64_t Fence::GetCompletedValue() const
{
	if (m_Fence == nullptr)
	{
		return 0;
	}

	return m_Fence->GetCompletedValue();
}
