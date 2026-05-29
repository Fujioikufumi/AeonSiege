#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include "ComPtr.h"
#include <vector>

class DescriptorHandle;
class DescriptorPool;

class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	bool Init(
		ID3D12Device* device,
		DescriptorPool* pool,
		size_t          size);

	bool Init(
		ID3D12Device* device,
		DescriptorPool* pool,
		size_t          size,
		uint32_t        framecount);

	void Term();

	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const;
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;
	[[nodiscard]] void* GetPtr() const;

	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetAddress(uint32_t frameIndex) const;
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU(uint32_t frameIndex) const;
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU(uint32_t frameIndex) const;
	[[nodiscard]] void* GetPtr(uint32_t frameIndex) const;

	template<typename T>
	[[nodiscard]] T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

	template<typename T>
	[[nodiscard]] T* GetPtr(uint32_t frameIndex)
	{
		return reinterpret_cast<T*>(GetPtr(frameIndex));
	}

private:
	// 単一CB用
	ComPtr<ID3D12Resource> m_CB;			// 定数バッファ
	DescriptorHandle* m_Handle;				// ディスクリプタハンドル
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc;	// 定数バッファビューの構成設定
	void* m_MappedPtr;						// マップ済みのポインタ

	// マルチフレームCB用
	std::vector<ComPtr<ID3D12Resource>>           m_CBs;
	std::vector<DescriptorHandle*>                m_Handles;
	std::vector<void*>                            m_MappedPtrs;
	std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> m_Descs;

	DescriptorPool* m_Pool;		// ディスクリプタプール

	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator = (const ConstantBuffer&) = delete;
};