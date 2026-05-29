#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "ComPtr.h"
#include <cstdint>
#include <vector>

class CommandList
{
public:
	CommandList();
	~CommandList();


	/// <summary>
	/// 初期化処理を行います.
	/// </summary>
	/// <param name="pDevice">デバイスです.</param>
	/// <param name="type">コマンドリストタイプです.</param>
	/// <param name="count">アロケータの数です. ダブルバッファ化する場合は2に設定します.</param>
	/// <returns>true 初期化に成功 : false 初期化に失敗</returns>
	bool Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count);

	/// <summary>
	/// 終了処理です.
	/// </summary>
	void Term();

	/// <summary>
	/// コマンドリストを取得します.
	/// </summary>
	/// <returns>リセット処理を行ったコマンドリストを返却します.</returns>
	[[nodiscard]] ID3D12GraphicsCommandList* Reset();

private:
	ComPtr<ID3D12GraphicsCommandList>			m_CmdList; // コマンドリスト
	std::vector<ComPtr<ID3D12CommandAllocator>> m_Allocators; // コマンドアロケータです.
	uint32_t                                    m_Index; // アロケータ番号です.

	CommandList		(const CommandList&) = delete;
	void operator = (const CommandList&) = delete;
};

