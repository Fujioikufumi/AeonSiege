//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Material.h"
#include "FileUtil.h"
#include "Logger.h"
#include "DescriptorPool.h"
#include "Texture.h"
#include "ConstantBuffer.h"

namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
    constexpr const wchar_t* DummyTag = L"";

}// namespace


///////////////////////////////////////////////////////////////////////////////
// Material class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Material::Material()
: m_Device(nullptr)
, m_Pool  (nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Material::~Material()
{ Term(); }


//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Material::Init
(
    ID3D12Device*   device,
    DescriptorPool* pool,
    size_t          bufferSize,
    size_t          count
)
{
    if (device == nullptr)
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    if(pool == nullptr)
    {
        ELOG("Error : DescriptorPool is null.");
        return false;
	}
    if(count == 0)
    {
        ELOG("Error : count is zero.");
        return false;
	}

    Term();

    m_Device = device;
    m_Device->AddRef();

    m_Pool = pool;
    m_Pool->AddRef();

    m_Subset.resize(count);

    // ダミーテクスチャを作成.
    D3D12_GPU_DESCRIPTOR_HANDLE dummyHandle = {};
    {
        auto texture = new (std::nothrow) Texture();
        if (texture == nullptr)
        {
            return false;
        }

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = 1;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        if (!texture->Init(
            device,
            pool,
            &desc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            false))
        {
            ELOG("Error : Texture::Init() Failed.");
            texture->Term();
            delete texture;
            return false;
        }

        m_Texture[DummyTag] = texture;
        dummyHandle = texture->GetHandleGPU();
    }

    // すべてのSubsetのテクスチャハンドルをダミーテクスチャで初期化（追加）
    for (size_t i = 0; i < count; ++i)
    {
        for (int usage = 0; usage < TEXTURE_USAGE_COUNT; ++usage)
        {
            m_Subset[i].TextureHandle[usage] = dummyHandle;
        }
    }

    auto size = bufferSize * count;
    if (size > 0)
    {
        for(size_t i=0; i<m_Subset.size(); ++i)
        {
            auto pBuffer = new (std::nothrow) ConstantBuffer();
            if (pBuffer == nullptr)
            {
                ELOG( "Error : Out of memory." );
                return false;
            }

            if (!pBuffer->Init(device, pool, bufferSize))
            {
                ELOG( "Error : ConstantBuffer::Init() Failed." );
                return false;
            }

            m_Subset[i].constantBuffer = pBuffer;
            for(auto j=0; j<TEXTURE_USAGE_COUNT; ++j)
            { m_Subset[i].TextureHandle[j].ptr = 0; }
        }
    }
    else
    {
        for(size_t i=0; i<m_Subset.size(); ++i)
        {
            m_Subset[i].constantBuffer = nullptr;
            for(auto j=0; j<TEXTURE_USAGE_COUNT; ++j)
            { m_Subset[i].TextureHandle[j].ptr = 0; }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Material::Term()
{
    for(auto& itr : m_Texture)
    {
        if (itr.second != nullptr)
        {
            itr.second->Term();
            delete itr.second;
            itr.second = nullptr;
        }
    }

    for(size_t i=0; i<m_Subset.size(); ++i)
    {
        if (m_Subset[i].constantBuffer != nullptr)
        {
            m_Subset[i].constantBuffer->Term();
            delete m_Subset[i].constantBuffer;
            m_Subset[i].constantBuffer = nullptr;
        }
    }

    m_Texture.clear();
    m_Subset.clear();

    if (m_Device != nullptr)
    {
        m_Device->Release();
        m_Device = nullptr;
    }

    if (m_Pool != nullptr)
    {
        m_Pool->Release();
        m_Pool = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      テクスチャを設定します.
//-----------------------------------------------------------------------------
bool Material::SetTexture
(
    size_t                          index,
    TEXTURE_USAGE                   usage,
    const std::wstring&             path,
    DirectX::ResourceUploadBatch&   batch
)
{
    // 範囲内であるかチェック.
    if (index >= GetCount())
    { return false; }

    // 既に登録済みかチェック.
    if (m_Texture.find(path) != m_Texture.end())
    {
        m_Subset[index].TextureHandle[usage] = m_Texture[path]->GetHandleGPU();
        return true;
    }

    // ファイルパスが存在するかチェックします.
    std::wstring findPath;
    if (!SearchFilePathW(path.c_str(), findPath))
    {
        // 存在しない場合はダミーテクスチャを設定.
        m_Subset[index].TextureHandle[usage] = m_Texture[DummyTag]->GetHandleGPU();
        return true;
    }

    // ファイル名であることをチェック.
    {
        if (PathIsDirectoryW(findPath.c_str()) != FALSE)
        {
            m_Subset[index].TextureHandle[usage] = m_Texture[DummyTag]->GetHandleGPU();
            return true;
        }
    }

    // インスタンス生成.
    auto pTexture = new (std::nothrow) Texture();
    if (pTexture == nullptr)
    {
        ELOG( "Error : Out of memory." );
        return false;
    }

    auto isSRGB = (TU_BASE_COLOR == usage) || (TU_DIFFUSE == usage) || (TU_SPECULAR == usage);

    // 初期化.
    if (!pTexture->Init(m_Device, m_Pool, findPath.c_str(), isSRGB, batch))
    {
        ELOG( "Error : Texture::Init() Failed." );
        pTexture->Term();
        delete pTexture;
        return false;
    }

    // 登録.
    m_Texture[path] = pTexture;
    m_Subset[index].TextureHandle[usage] = pTexture->GetHandleGPU();

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      定数バッファのポインタを取得します.
//-----------------------------------------------------------------------------
void* Material::GetBufferPtr(size_t index) const
{
    if(index >= GetCount())
    { return nullptr; }

    return m_Subset[index].constantBuffer->GetPtr();
}

//-----------------------------------------------------------------------------
//      定数バッファのアドレスを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS Material::GetBufferAddress(size_t index) const
{
    if (index >= GetCount())
    { return D3D12_GPU_VIRTUAL_ADDRESS(); }

    return m_Subset[index].constantBuffer->GetAddress();
}

//-----------------------------------------------------------------------------
//      定数バッファハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetBufferHandle(size_t index) const
{
    if (index >= GetCount())
    { return D3D12_GPU_DESCRIPTOR_HANDLE(); }

    return m_Subset[index].constantBuffer->GetHandleGPU();
}

//-----------------------------------------------------------------------------
//      テクスチャハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetTextureHandle(size_t index, TEXTURE_USAGE usage) const
{
    if (index >= GetCount())
    { return D3D12_GPU_DESCRIPTOR_HANDLE(); }

    return m_Subset[index].TextureHandle[usage];
}

//-----------------------------------------------------------------------------
//      マテリアル数を取得します.
//-----------------------------------------------------------------------------
size_t Material::GetCount() const
{ return m_Subset.size(); }
