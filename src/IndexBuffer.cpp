#include "IndexBuffer.h"
#include <cstring>

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
IndexBuffer::IndexBuffer()
	: m_IndexBuffer(nullptr)
	, m_IndexCount(0)
{
	std::memset(&m_View, 0, sizeof(m_View));
}

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
	Term();
}

//-----------------------------------------------------------------------------
//      初期化
//-----------------------------------------------------------------------------
bool IndexBuffer::Init(ID3D12Device* device, size_t indexCount, const uint32_t* initData)
{
	if (device == nullptr || indexCount == 0)
	{
		return false;
	}

	// ヒーププロパティ
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// インデックスバッファ全体のファイルサイズ
	const UINT64 totalSize = static_cast<UINT64>(indexCount * sizeof(uint32_t));

	// リソース設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = totalSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// リソースの生成
	HRESULT result = device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_IndexBuffer.GetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	// インデックスバッファビューの設定
	m_View.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_View.Format = DXGI_FORMAT_R32_UINT;
	m_View.SizeInBytes = static_cast<UINT>(totalSize);

	// 初期値コピー
	if (initData != nullptr)
	{
		uint32_t* mappedPtr = Map();
		if (mappedPtr == nullptr)
		{
			return false;
		}

		std::memcpy(mappedPtr, initData, totalSize);
		Unmap();
	}

	m_IndexCount = indexCount;

	return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void IndexBuffer::Term()
{
	m_IndexBuffer.Reset();
	std::memset(&m_View, 0, sizeof(m_View));
	m_IndexCount = 0;
}

//-----------------------------------------------------------------------------
//      マッピング
//-----------------------------------------------------------------------------
uint32_t* IndexBuffer::Map()
{
	if (m_IndexBuffer == nullptr)
	{
		return nullptr;
	}

	uint32_t* ptr = nullptr;
	HRESULT result = m_IndexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
	if (FAILED(result))
	{
		return nullptr;
	}

	return ptr;
}

//-----------------------------------------------------------------------------
//      アンマッピング
//-----------------------------------------------------------------------------
void IndexBuffer::Unmap()
{
	if (m_IndexBuffer != nullptr)
	{
		m_IndexBuffer->Unmap(0, nullptr);
	}
}

//-----------------------------------------------------------------------------
//      インデックスバッファビュー取得
//-----------------------------------------------------------------------------
D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const
{
	return m_View;
}

//-----------------------------------------------------------------------------
//      インデックス数取得
//-----------------------------------------------------------------------------
size_t IndexBuffer::GetCount() const
{
	return m_IndexCount;
}