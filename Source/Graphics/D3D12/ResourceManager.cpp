#include "Graphics/D3D12/ResourceManager.h"
#include "Utility/Logger.h"
#include "Graphics/D3D12/Texture.h"
#include "Utility/FileUtil.h"
#include "ResourceUploadBatch.h"

bool ResourceManager::Init(
    ComPtr<ID3D12Device> device,
    DescriptorPool* pools[POOL_COUNT],
    ComPtr<ID3D12CommandQueue> queue)
{
	m_Device = device;
	m_Queue  = queue;

	for (int i = 0; i < POOL_COUNT; ++i)
	{
		m_Pool[i] = pools[i];
	}

	m_RenderSystem = nullptr;

	return true;
}

void ResourceManager::Term()
{
	ClearTextureCache();
	ReleaseRetiredResources(UINT64_MAX);

	m_Device.Reset();
	m_Queue.Reset();

	for (int i = 0; i < POOL_COUNT; ++i)
	{
		m_Pool[i] = nullptr;
	}
	m_RenderSystem = nullptr;
}

void ResourceManager::SetCurrentFenceValue(uint64_t fenceValue)
{
	m_CurrentFenceValue = fenceValue;
}

void ResourceManager::RetireResource(ComPtr<ID3D12Resource>& resource)
{
	if (resource.Get() == nullptr)
	{
		return;
	}

	RetiredResource retired;
	retired.Resource   = std::move(resource);
	retired.FenceValue = m_CurrentFenceValue;
	m_RetiredResources.push_back(std::move(retired));
}

void ResourceManager::RetireDescriptor(DescriptorPool* pool, DescriptorHandle*& handle)
{
	if (pool == nullptr || handle == nullptr)
	{
		return;
	}

	pool->AddRef();

	RetiredDescriptor retired;
	retired.Pool       = pool;
	retired.Handle     = handle;
	retired.FenceValue = m_CurrentFenceValue;
	m_RetiredDescriptors.push_back(retired);

	handle = nullptr;
}

void ResourceManager::ReleaseRetiredResources(uint64_t completedFenceValue)
{
	for (auto it = m_RetiredDescriptors.begin(); it != m_RetiredDescriptors.end();)
	{
		if (it->FenceValue == 0 || it->FenceValue <= completedFenceValue)
		{
			it->Pool->FreeHandle(it->Handle);
			it->Pool->Release();
			it = m_RetiredDescriptors.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = m_RetiredResources.begin(); it != m_RetiredResources.end();)
	{
		if (it->FenceValue == 0 || it->FenceValue <= completedFenceValue)
		{
			it = m_RetiredResources.erase(it);
		}
		else
		{
			++it;
		}
	}
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(
    const std::wstring& texturePath,
    bool isSRGB)
{
	const std::wstring cacheKey = texturePath + (isSRGB ? L"#srgb" : L"#linear");

	auto it = m_TextureCache.find(cacheKey);
	if (it != m_TextureCache.end())
	{
		return it->second;
	}

	std::wstring fullPath;
	if (!SearchFilePath(texturePath.c_str(), fullPath))
	{
		ELOG("Error : Texture file not found. path = %ls", texturePath.c_str());
		return nullptr;
	}

	DescriptorPool* pool = GetPool(POOL_TYPE_RES);
	if (m_Device.Get() == nullptr || m_Queue.Get() == nullptr || pool == nullptr)
	{
		ELOG("Error : ResourceManager is not initialized.");
		return nullptr;
	}

	auto texture = std::make_shared<Texture>();

	DirectX::ResourceUploadBatch batch(m_Device.Get());
	batch.Begin();

	if (!texture->Init(m_Device.Get(), pool, fullPath.c_str(), isSRGB, batch))
	{
		ELOG("Error : Texture::Init() Failed. path = %ls", fullPath.c_str());
		return nullptr;
	}

	auto finish = batch.End(m_Queue.Get());
	finish.wait();

	m_TextureCache.emplace(cacheKey, texture);
	return texture;
}

void ResourceManager::ClearTextureCache()
{
	m_TextureCache.clear();
}
