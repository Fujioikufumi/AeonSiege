#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <map>
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <ResourceUploadBatch.h>

//-----------------------------------------------------------------------------
// Forward Declarations.
//-----------------------------------------------------------------------------
class Texture;
class ConstantBuffer;
class DescriptorPool;


///////////////////////////////////////////////////////////////////////////////
// Material class
///////////////////////////////////////////////////////////////////////////////
class Material
{
public:
    ///////////////////////////////////////////////////////////////////////////
    // TEXTURE_USAGE enum
    ///////////////////////////////////////////////////////////////////////////
    enum TEXTURE_USAGE
    {
        TEXTURE_USAGE_DIFFUSE = 0,  // ディフューズマップ
        TEXTURE_USAGE_SPECULAR,     // スペキュラーマップ
        TEXTURE_USAGE_SHININESS,    // シャイニネスマップ
        TEXTURE_USAGE_NORMAL,       // 法線マップ

        TEXTURE_USAGE_BASE_COLOR,   // ベースカラーマップ
        TEXTURE_USAGE_METALLIC,     // メタリックマップ
        TEXTURE_USAGE_ROUGHNESS,    // ラフネスマップ

        TEXTURE_USAGE_COUNT
    };

    Material();
    ~Material();

    Material(Material&& other) noexcept
        : m_Texture(std::move(other.m_Texture))
        , m_Subset(std::move(other.m_Subset))
        , m_Device(other.m_Device)
        , m_Pool(other.m_Pool)
    {
        other.m_Device = nullptr;
        other.m_Pool = nullptr;
    }

    Material& operator=(Material&& other) noexcept
    {
        if (this != &other)
        {
            Term();

            m_Texture = std::move(other.m_Texture);
            m_Subset = std::move(other.m_Subset);
            m_Device = other.m_Device;
            m_Pool = other.m_Pool;

            other.m_Device = nullptr;
            other.m_Pool = nullptr;
        }
        return *this;
    }

    bool Init(
        ID3D12Device* device,
        DescriptorPool* pool,
        size_t          bufferSize,
        size_t          count);

    void Term();

    bool SetTexture(
        size_t                          index,
        TEXTURE_USAGE                   usage,
        const std::wstring& path,
        DirectX::ResourceUploadBatch& batch);

    [[nodiscard]] void* GetBufferPtr(size_t index) const;

    template<typename T>
    [[nodiscard]] T* GetBufferPtr(size_t index) const
    {
        return reinterpret_cast<T*>(GetBufferPtr(index));
    }

    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress(size_t index) const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetBufferHandle(size_t index) const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(size_t index, TEXTURE_USAGE usage) const;
    [[nodiscard]] size_t GetCount() const;

private:
    ///////////////////////////////////////////////////////////////////////////
    // Subset structure
    ///////////////////////////////////////////////////////////////////////////
    struct Subset
    {
        ConstantBuffer* constantBuffer;                     // 定数バッファ
        D3D12_GPU_DESCRIPTOR_HANDLE     TextureHandle[TEXTURE_USAGE_COUNT]; // テクスチャハンドル
    };

    std::map<std::wstring, Texture*>    m_Texture;      // テクスチャ
    std::vector<Subset>                 m_Subset;       // サブセット
    ID3D12Device* m_Device;       // デバイス
    DescriptorPool* m_Pool;         // ディスクリプタプール

    Material(const Material&) = delete;
    void operator = (const Material&) = delete;
};

constexpr auto TU_DIFFUSE = Material::TEXTURE_USAGE_DIFFUSE;
constexpr auto TU_SPECULAR = Material::TEXTURE_USAGE_SPECULAR;
constexpr auto TU_SHININESS = Material::TEXTURE_USAGE_SHININESS;
constexpr auto TU_NORMAL = Material::TEXTURE_USAGE_NORMAL;

constexpr auto TU_BASE_COLOR = Material::TEXTURE_USAGE_BASE_COLOR;
constexpr auto TU_METALLIC = Material::TEXTURE_USAGE_METALLIC;
constexpr auto TU_ROUGHNESS = Material::TEXTURE_USAGE_ROUGHNESS;


