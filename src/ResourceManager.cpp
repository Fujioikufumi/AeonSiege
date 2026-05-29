#include "ResourceManager.h"
#include "Logger.h"


bool ResourceManager::Init(
    ComPtr<ID3D12Device> device,
    DescriptorPool* pools[POOL_COUNT],
    ComPtr<ID3D12CommandQueue> queue)
{
    m_Device = device;
    m_Queue = queue;

    for (int i = 0; i < POOL_COUNT; ++i)
    {
        m_Pool[i] = pools[i];
    }

    m_RenderSystem = nullptr;

    return true;
}

void ResourceManager::Term()
{
    m_Device.Reset();
    m_Queue.Reset();

    for (int i = 0; i < POOL_COUNT; ++i)
    {
        m_Pool[i] = nullptr;
    }
    m_RenderSystem = nullptr;
}