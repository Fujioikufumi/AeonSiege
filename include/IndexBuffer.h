#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "ComPtr.h"
#include <cstdint>

/// <summary>
/// インデックスバッファを管理するクラス。
/// </summary>
class IndexBuffer
{
public:
	IndexBuffer();
	~IndexBuffer();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="device">Direct3D 12 デバイス</param>
	/// <param name="indexCount">インデックス数</param>
	/// <param name="initData">初期化データ（任意）</param>
	/// <returns>成功したら true</returns>
	bool Init(ID3D12Device* device, size_t indexCount, const uint32_t* initData = nullptr);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();

	/// <summary>
	/// CPUメモリへのマッピング
	/// </summary>
	/// <returns>マップされたポインタ</returns>
	[[nodiscard]] uint32_t* Map();

	/// <summary>
	/// マッピング解除
	/// </summary>
	void Unmap();

	/// <summary>
	/// インデックスバッファビューを取得
	/// </summary>
	[[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetView() const;

	/// <summary>
	/// インデックス数を取得
	/// </summary>
	[[nodiscard]] size_t GetCount() const;

private:
	ComPtr<ID3D12Resource>  m_IndexBuffer; // インデックスバッファリソース
	D3D12_INDEX_BUFFER_VIEW m_View;        // インデックスバッファビュー
	size_t                  m_IndexCount;  // インデックス数

	IndexBuffer(const IndexBuffer&) = delete;
	void operator=(const IndexBuffer&) = delete;
};