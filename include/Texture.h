#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <ComPtr.h>
#include <ResourceUploadBatch.h>

//-----------------------------------------------------------------------------
// Forward Declarations.
//-----------------------------------------------------------------------------
class DescriptorHandle;
class DescriptorPool;

///////////////////////////////////////////////////////////////////////////////
// Texture class
///////////////////////////////////////////////////////////////////////////////
class Texture
{
public:
    Texture();
    ~Texture();

    bool Init(
        ID3D12Device* device,
        DescriptorPool* pool,
        const wchar_t* filename,
        DirectX::ResourceUploadBatch& batch);

    bool Init(
        ID3D12Device* device,
        DescriptorPool* pool,
        const wchar_t* filename,
        bool                          isSRGB,
        DirectX::ResourceUploadBatch& batch);

    bool Init(
        ID3D12Device* device,
        DescriptorPool* pool,
        const D3D12_RESOURCE_DESC* resourceDesc,
        D3D12_RESOURCE_STATES      initState,
        bool                       isCube);

    bool Init(
        ID3D12Device* device,
        DescriptorPool* pool,
        const D3D12_RESOURCE_DESC* resourceDesc,
        D3D12_RESOURCE_STATES      initState,
        bool                       isCube,
        bool                       isSRGB);

    void Term();

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;
    [[nodiscard]] ID3D12Resource* GetResource() const;

private:
    ComPtr<ID3D12Resource>  m_Texture;
    DescriptorHandle* m_Handle;
    DescriptorPool* m_Pool;

    Texture(const Texture&) = delete;
    void operator = (const Texture&) = delete;

    [[nodiscard]] D3D12_SHADER_RESOURCE_VIEW_DESC GetViewDesc(bool isCube) const;
};