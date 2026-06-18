//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ConstantBuffer.h"
#include "DescriptorPool.h"
#include "RootSignature.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ConstantBuffer::ConstantBuffer()
: m_CB         (nullptr)
, m_Handle     (nullptr)
, m_Pool       (nullptr)
, m_MappedPtr  (nullptr)
, m_Desc       ( 0 )
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ConstantBuffer::~ConstantBuffer()
{ 
    Term(); 
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ConstantBuffer::Init
(
    ID3D12Device* pDevice,
    DescriptorPool* pPool,
    size_t          size
)
{
    if (pDevice == nullptr || pPool == nullptr || size == 0)
    {
        ELOG("Error : Invalid arguments to ConstantBuffer::Init()");
        return false;
    }

    // 既に初期化されている場合は終了処理を実行
    if (m_CB.Get() != nullptr || m_MappedPtr != nullptr)
    {
        ELOG("Warning : ConstantBuffer is already initialized. Calling Term() first.");
        Term();

        // Term()の後、すべてがnullptrになっているか確認
        if (m_CB.Get() != nullptr || m_MappedPtr != nullptr || m_Handle != nullptr)
        {
            ELOG("Error : ConstantBuffer::Term() did not properly clean up.");
            ELOG("  m_CB = %p, m_MappedPtr = %p, m_Handle = %p",
                m_CB.Get(), m_MappedPtr, m_Handle);
            return false;
        }
    }

    assert(m_Pool == nullptr);
    assert(m_Handle == nullptr);

    m_Pool = pPool;
    m_Pool->AddRef();

    size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    UINT64 sizeAligned = (size + (align - 1)) & ~(align - 1);

    // ヒーププロパティ.
    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type = D3D12_HEAP_TYPE_UPLOAD;
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    // リソースの設定.
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = sizeAligned;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // リソースを生成.
    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_CB.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. hr = 0x%x, size = %zu, sizeAligned = %llu",
            hr, size, sizeAligned);
        return false;
    }

    // リソースが正しく作成されたか確認
    if (m_CB.Get() == nullptr)
    {
        ELOG("Error : m_CB is nullptr after CreateCommittedResource");
        return false;
    }

    // リソースの詳細情報を確認
    D3D12_RESOURCE_DESC resourceDesc = m_CB->GetDesc();
    /*DLOG("ConstantBuffer resource created: Width = %llu, Dimension = %d, Flags = %d",
        resourceDesc.Width, resourceDesc.Dimension, resourceDesc.Flags);*/

    // メモリマッピングしておきます.
    // 書き込みのみの場合は、pReadRangeにnullptrを渡す
    hr = m_CB->Map(0, nullptr, &m_MappedPtr);
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Resource::Map() Failed. hr = 0x%x (0x887a0005 = DXGI_ERROR_INVALID_CALL)", hr);
        ELOG("  size = %zu, sizeAligned = %llu", size, sizeAligned);
        ELOG("  Resource Desc: Width = %llu, Dimension = %d, Flags = %d",
            resourceDesc.Width, resourceDesc.Dimension, resourceDesc.Flags);

        // リソースの状態を確認
        // 注意: GetResourceState()は存在しないので、別の方法で確認
        ELOG("  Attempting to get resource state information...");

        // リソースを解放
        m_CB.Reset();
        return false;
    }

    // マッピングされたポインタが有効か確認
    if (m_MappedPtr == nullptr)
    {
        ELOG("Error : m_MappedPtr is nullptr after Map()");
        m_CB.Reset();
        return false;
    }

    m_Desc.BufferLocation = m_CB->GetGPUVirtualAddress();
    m_Desc.SizeInBytes = UINT(sizeAligned);
    m_Handle = pPool->AllocHandle();

    if (m_Handle == nullptr)
    {
        ELOG("Error : DescriptorPool::AllocHandle() returned nullptr. Pool may be full.");
        m_CB->Unmap(0, nullptr);
        m_CB.Reset();
        return false;
    }

    pDevice->CreateConstantBufferView(&m_Desc, m_Handle->HandleCPU);

    // 正常終了.
    return true;
}

bool ConstantBuffer::Init(ID3D12Device* pDevice, DescriptorPool* pPool, size_t size, uint32_t framecount)
{
    if( !pDevice || !pPool || size == 0 || framecount == 0 )
	{ return false; }

    assert(m_Pool == nullptr);
    m_Pool = pPool; 
    m_Pool->AddRef();

    size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    UINT64 sizeAligned = (size + (align - 1)) & ~(align - 1);

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type = D3D12_HEAP_TYPE_UPLOAD;
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = sizeAligned;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    m_CBs.resize(framecount);
    m_Handles.resize(framecount);
    m_MappedPtrs.resize(framecount);
    m_Descs.resize(framecount);

    for (uint32_t i = 0; i < framecount; ++i)
    {
        auto& cb = m_CBs[i];
        HRESULT hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(cb.GetAddressOf()));
        if (FAILED(hr)) return false;

        hr = cb->Map(0, nullptr, &m_MappedPtrs[i]);
        if (FAILED(hr)) return false;

        m_Descs[i].BufferLocation = cb->GetGPUVirtualAddress();
        m_Descs[i].SizeInBytes = static_cast<UINT>(sizeAligned);
        m_Handles[i] = m_Pool->AllocHandle();

        pDevice->CreateConstantBufferView(&m_Descs[i], m_Handles[i]->HandleCPU);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ConstantBuffer::Term() // 単一と多フレーム両方の終了処理を行います.
{
    // メモリマッピングを解除して，定数バッファを解放します.
    if (m_CB) {
        m_CB->Unmap(0, nullptr);
        m_CB.Reset();
    }
	m_MappedPtr = nullptr;

    if (m_Handle && m_Pool) {
        m_Pool->FreeHandle(m_Handle);
        m_Handle = nullptr;
    }

    for (size_t i = 0; i < m_CBs.size(); ++i)
    {
        if (m_CBs[i]) {
            m_CBs[i]->Unmap(0, nullptr);
            m_CBs[i].Reset();
        }
        if (m_Handles[i] && m_Pool) {
            m_Pool->FreeHandle(m_Handles[i]);
            m_Handles[i] = nullptr;
        }
    }
    m_CBs.clear();
    m_Handles.clear();
    m_MappedPtrs.clear();
    m_Descs.clear();

    if (m_Pool) {
        m_Pool->Release();
        m_Pool = nullptr;
    }
    m_MappedPtr = nullptr;
}

//-----------------------------------------------------------------------------
//      GPU仮想アドレスを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetAddress() const
{ return m_Desc.BufferLocation; }

//-----------------------------------------------------------------------------
//      CPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetHandleCPU() const
{
    if (m_Handle == nullptr)
    { return D3D12_CPU_DESCRIPTOR_HANDLE(); }

    return m_Handle->HandleCPU;
}

//-----------------------------------------------------------------------------
//      GPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE ConstantBuffer::GetHandleGPU() const
{

    if (m_Handle == nullptr)
    { return D3D12_GPU_DESCRIPTOR_HANDLE(); }

    return m_Handle->HandleGPU;
}


//-----------------------------------------------------------------------------
//      メモリマッピング済みポインタを取得します.
//-----------------------------------------------------------------------------
void* ConstantBuffer::GetPtr() const
{ return m_MappedPtr; }



D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetAddress(uint32_t frameIndex) const
{ return m_Descs[frameIndex].BufferLocation; }

D3D12_GPU_DESCRIPTOR_HANDLE ConstantBuffer::GetHandleGPU(uint32_t frameIndex) const
{ return m_Handles[frameIndex] ? m_Handles[frameIndex]->HandleGPU : D3D12_GPU_DESCRIPTOR_HANDLE(); }

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::GetHandleCPU(uint32_t frameIndex) const
{ return m_Handles[frameIndex] ? m_Handles[frameIndex]->HandleCPU : D3D12_CPU_DESCRIPTOR_HANDLE(); }

void* ConstantBuffer::GetPtr(uint32_t frameIndex) const
{ return m_MappedPtrs[frameIndex]; }
