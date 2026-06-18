//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "CommandList.h"


///////////////////////////////////////////////////////////////////////////////
// CommandList class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
CommandList::CommandList()
: m_CmdList    (nullptr)
, m_Allocators ()
, m_Index       (0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
CommandList::~CommandList()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool CommandList::Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, uint32_t count)
{
    if (pDevice == nullptr || count == 0)
    { return false; }

    m_Allocators.resize(count);

    for(auto i=0u; i<count; ++i)
    {
        auto hr = pDevice->CreateCommandAllocator(
            type, IID_PPV_ARGS(m_Allocators[i].GetAddressOf()));
        if (FAILED(hr))
        { return false; }
    }

    {
        auto hr = pDevice->CreateCommandList(
            1,
            type,
            m_Allocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(m_CmdList.GetAddressOf()));
        if (FAILED(hr))
        { return false; }

        m_CmdList->Close();
    }

    m_Index = 0;
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void CommandList::Term()
{
    m_CmdList.Reset();

    for(size_t i=0; i<m_Allocators.size(); ++i)
    { m_Allocators[i].Reset(); }

    m_Allocators.clear();
    m_Allocators.shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      リセット処理を行います.
//-----------------------------------------------------------------------------
ID3D12GraphicsCommandList* CommandList::Reset()
{
    auto hr = m_Allocators[m_Index]->Reset();
    if (FAILED(hr))
    { return nullptr; }

    hr = m_CmdList->Reset(m_Allocators[m_Index].Get(), nullptr);
    if (FAILED(hr))
    { return nullptr; }

    m_Index = (m_Index + 1) % uint32_t(m_Allocators.size());
    return m_CmdList.Get();
}

