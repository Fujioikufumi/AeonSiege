#include <DescriptorPool.h>
#include <new>

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
DescriptorPool::DescriptorPool()
	: m_RefCount(1)
	, m_Pool()
	, m_Heap()
	, m_DescriptorSize(0)
{
}

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
DescriptorPool::~DescriptorPool()
{
	m_Pool.Term();
	m_Heap.Reset();
	m_DescriptorSize = 0;
}

//-----------------------------------------------------------------------------
//      参照カウント追加
//-----------------------------------------------------------------------------
void DescriptorPool::AddRef()
{
	m_RefCount++;
}

//-----------------------------------------------------------------------------
//      解放
//-----------------------------------------------------------------------------
void DescriptorPool::Release()
{
	m_RefCount--;
	if (m_RefCount == 0)
	{
		delete this;
	}
}

//-----------------------------------------------------------------------------
//      カウント取得
//-----------------------------------------------------------------------------
uint32_t DescriptorPool::GetCount() const
{
	return m_RefCount.load();
}

//-----------------------------------------------------------------------------
//      ハンドル割り当て
//-----------------------------------------------------------------------------
DescriptorHandle* DescriptorPool::AllocHandle()
{
	auto func = [&](uint32_t index, DescriptorHandle* handle)
		{
			auto handleCPU = m_Heap->GetCPUDescriptorHandleForHeapStart();
			handleCPU.ptr += static_cast<size_t>(m_DescriptorSize) * index;

			D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
			if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
			{
				auto handleGPU = m_Heap->GetGPUDescriptorHandleForHeapStart();
				handleGPU.ptr += static_cast<size_t>(m_DescriptorSize) * index;
				handle->HandleGPU = handleGPU;
			}
			else
			{
				handle->HandleGPU.ptr = 0;
			}

			handle->HandleCPU = handleCPU;
		};

	return m_Pool.Alloc(func);
}

//-----------------------------------------------------------------------------
//      ハンドル解放
//-----------------------------------------------------------------------------
void DescriptorPool::FreeHandle(DescriptorHandle*& handle)
{
	if (handle != nullptr)
	{
		m_Pool.Free(handle);
		handle = nullptr;
	}
}

//-----------------------------------------------------------------------------
//	    空きハンドル数
//-----------------------------------------------------------------------------
uint32_t DescriptorPool::GetAvailableHandleCount() const
{
	return m_Pool.GetAvailableCount();
}

//-----------------------------------------------------------------------------
// 	    使用中ハンドル数
//-----------------------------------------------------------------------------
uint32_t DescriptorPool::GetAllocatedHandleCount() const
{
	return m_Pool.GetUsedCount();
}

//-----------------------------------------------------------------------------
// 	    総ハンドル数
//-----------------------------------------------------------------------------
uint32_t DescriptorPool::GetHandleCount() const
{
	return m_Pool.GetSize();
}

//-----------------------------------------------------------------------------
// 	    ヒープ取得
//-----------------------------------------------------------------------------
ID3D12DescriptorHeap* const DescriptorPool::GetHeap() const
{
	return m_Heap.Get();
}

//-----------------------------------------------------------------------------
// 	    生成
//-----------------------------------------------------------------------------
bool DescriptorPool::Create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* desc, DescriptorPool** poolOut)
{
	if (device == nullptr || desc == nullptr || poolOut == nullptr)
	{
		return false;
	}

	auto instance = new (std::nothrow) DescriptorPool();
	if (instance == nullptr)
	{
		return false;
	}

	HRESULT result = device->CreateDescriptorHeap(desc, IID_PPV_ARGS(instance->m_Heap.GetAddressOf()));
	if (FAILED(result))
	{
		delete instance;
		return false;
	}

	if (!instance->m_Pool.Init(desc->NumDescriptors))
	{
		delete instance;
		return false;
	}

	instance->m_DescriptorSize = device->GetDescriptorHandleIncrementSize(desc->Type);
	*poolOut = instance;

	return true;
}