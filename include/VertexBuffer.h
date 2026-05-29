#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <ComPtr.h>
#include <utility>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
// VertexBuffer class
///////////////////////////////////////////////////////////////////////////////
class VertexBuffer
{
public:
    VertexBuffer();
    ~VertexBuffer();

    VertexBuffer(VertexBuffer&& other) noexcept
        : m_VertexBuffer(std::move(other.m_VertexBuffer))
        , m_View(other.m_View)
    {
        other.m_VertexBuffer = nullptr;
        std::memset(&other.m_View, 0, sizeof(other.m_View));
    }

    VertexBuffer& operator=(VertexBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Term();

            m_VertexBuffer = std::move(other.m_VertexBuffer);
            m_View = other.m_View;

            other.m_VertexBuffer = nullptr;
            std::memset(&other.m_View, 0, sizeof(other.m_View));
        }
        return *this;
    }

    // 初期化処理
    bool Init(
        ID3D12Device* device,
        size_t        size,
        size_t        stride,
        const void* initData = nullptr);

    // 型指定ありの初期化
    template<typename T>
    bool Init(ID3D12Device* device, size_t count, const T* initData = nullptr)
    {
        return Init(device, sizeof(T) * count, sizeof(T), initData);
    }

    // 終了処理
    void Term();

    // メモリのマッピング
    [[nodiscard]] void* Map() const;

    // メモリのアンマッピング
    void Unmap();

    // 型指定ありのマッピング
    template<typename T>
    [[nodiscard]] T* Map() const
    {
        return static_cast<T*>(Map());
    }

    // 頂点バッファビューの取得
    [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_View; }

    // リソースの取得
    [[nodiscard]] ID3D12Resource* GetResource() const { return m_VertexBuffer.Get(); }

private:
    ComPtr<ID3D12Resource>   m_VertexBuffer; // 頂点バッファリソース
    D3D12_VERTEX_BUFFER_VIEW m_View;         // 頂点バッファビュー

    VertexBuffer(const VertexBuffer&) = delete;
    void operator=(const VertexBuffer&) = delete;
};