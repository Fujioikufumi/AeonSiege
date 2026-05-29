#pragma once
//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <d3d12.h>
#include <atomic>
#include <ComPtr.h>
#include <Pool.h>

///////////////////////////////////////////////////////////////////////////////
// DescriptorHandle class
///////////////////////////////////////////////////////////////////////////////
class DescriptorHandle
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU;  //!< CPUディスクリプタハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU;  //!< GPUディスクリプタハンドル

    [[nodiscard]] bool HasCPU() const
    {
        return HandleCPU.ptr != 0;
    }

    [[nodiscard]] bool HasGPU() const
    {
        return HandleGPU.ptr != 0;
    }
};

///////////////////////////////////////////////////////////////////////////////
// DescriptorPool class
///////////////////////////////////////////////////////////////////////////////
class DescriptorPool
{
public:
    /// <summary>
    /// 生成処理を行います
    /// </summary>
    /// <param name="device">デバイス</param>
    /// <param name="desc">ディスクリプタヒープの設定</param>
    /// <param name="poolOut">生成されたプールの格納先</param>
    /// <returns>成功したら true</returns>
    static bool Create(
        ID3D12Device* device,
        const D3D12_DESCRIPTOR_HEAP_DESC* desc,
        DescriptorPool** poolOut);

    /// <summary>
    /// 参照カウントを増やします
    /// </summary>
    void AddRef();

    /// <summary>
    /// 解放処理を行います
    /// </summary>
    void Release();

    /// <summary>
    /// 参照カウントを取得します
    /// </summary>
    [[nodiscard]] uint32_t GetCount() const;

    /// <summary>
    /// ディスクリプタハンドルを割り当てます
    /// </summary>
    [[nodiscard]] DescriptorHandle* AllocHandle();

    /// <summary>
    /// ディスクリプタハンドルを解放します
    /// </summary>
    void FreeHandle(DescriptorHandle*& handle);

    /// <summary>
    /// 利用可能なハンドル数を取得します
    /// </summary>
    [[nodiscard]] uint32_t GetAvailableHandleCount() const;

    /// <summary>
    /// 割り当て済みのハンドル数を取得します
    /// </summary>
    [[nodiscard]] uint32_t GetAllocatedHandleCount() const;

    /// <summary>
    /// ハンドル総数を取得します
    /// </summary>
    [[nodiscard]] uint32_t GetHandleCount() const;

    /// <summary>
    /// ディスクリプタヒープを取得します
    /// </summary>
    [[nodiscard]] ID3D12DescriptorHeap* const GetHeap() const;

private:
    std::atomic<uint32_t>           m_RefCount;         //!< 参照カウント
    Pool<DescriptorHandle>          m_Pool;             //!< ディスクリプタハンドルプール
    ComPtr<ID3D12DescriptorHeap>    m_Heap;             //!< ディスクリプタヒープ
    uint32_t                        m_DescriptorSize;   //!< ディスクリプタサイズ

    DescriptorPool();
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    void operator = (const DescriptorPool&) = delete;
};