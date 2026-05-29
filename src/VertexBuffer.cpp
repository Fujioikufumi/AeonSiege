#include "VertexBuffer.h"

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
VertexBuffer::VertexBuffer()
	: m_VertexBuffer(nullptr)
{
	std::memset(&m_View, 0, sizeof(m_View));
}

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
VertexBuffer::~VertexBuffer()
{
	Term();
}

//-----------------------------------------------------------------------------
//      初期化
//-----------------------------------------------------------------------------
bool VertexBuffer::Init(
	ID3D12Device* device,
	size_t        size,
	size_t        stride,
	const void* initData)
{
	if (device == nullptr || size == 0 || stride == 0)
	{
		return false;
	}

	// ヒーププロパティの設定
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// リソース設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = static_cast<UINT64>(size);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// リソース生成
	HRESULT result = device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_VertexBuffer.GetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	// ビューの構成
	m_View.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_View.StrideInBytes = static_cast<UINT>(stride);
	m_View.SizeInBytes = static_cast<UINT>(size);

	// 初期データがあればコピー
	if (initData != nullptr)
	{
		void* mappedPtr = Map();
		if (mappedPtr == nullptr)
		{
			return false;
		}

		std::memcpy(mappedPtr, initData, size);
		Unmap();
	}

	return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void VertexBuffer::Term()
{
	m_VertexBuffer.Reset();
	std::memset(&m_View, 0, sizeof(m_View));
}

//-----------------------------------------------------------------------------
//      マッピング
//-----------------------------------------------------------------------------
void* VertexBuffer::Map() const
{
	if (m_VertexBuffer == nullptr)
	{
		return nullptr;
	}

	void* ptr = nullptr;
	HRESULT result = m_VertexBuffer->Map(0, nullptr, &ptr);
	if (FAILED(result))
	{
		return nullptr;
	}
	return ptr;
}

//-----------------------------------------------------------------------------
//      アンマッピング
//-----------------------------------------------------------------------------
void VertexBuffer::Unmap()
{
	if (m_VertexBuffer != nullptr)
	{
		m_VertexBuffer->Unmap(0, nullptr);
	}
}