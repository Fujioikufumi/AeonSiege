#pragma once
#include "Core/NameSpace.h"
#include "Graphics/D3D12/DescriptorPool.h"
#include <d3d12.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "EncodingUtils/ComPtr.h"
#include "Core/SingletonManager.h"

class RenderSystem;
class Texture;

/// <summary>
/// DirectX12 デバイスやデスクリプタプールなどの低レイヤーリソースを中枢管理するシングルトンクラス。
/// </summary>
class ResourceManager : public SingletonManager<ResourceManager>
{
private:
	friend class SingletonManager<ResourceManager>;

	ComPtr<ID3D12Device> m_Device;
	DescriptorPool* m_Pool[POOL_COUNT] = {nullptr};
	ComPtr<ID3D12CommandQueue> m_Queue;
	RenderSystem* m_RenderSystem = nullptr;

	ResourceManager()  = default;
	~ResourceManager() = default;

	// リソース解放のためにフェンス値と共に保持する構造体
	struct RetiredResource
	{
		ComPtr<ID3D12Resource> Resource;
		uint64_t FenceValue = 0;
	};

	// ディスクリプタ解放のためにフェンス値と共に保持する構造体
	struct RetiredDescriptor
	{
		DescriptorPool* Pool     = nullptr;
		DescriptorHandle* Handle = nullptr;
		uint64_t FenceValue      = 0;
	};

	uint64_t m_CurrentFenceValue = 0;					 // 現在のフェンス値
	std::vector<RetiredResource> m_RetiredResources;	 // 解放待ちのリソース
	std::vector<RetiredDescriptor> m_RetiredDescriptors; // 解放待ちのディスクリプタ
	std::unordered_map<std::wstring, std::shared_ptr<Texture>> m_TextureCache; // テクスチャキャッシュ

public:
	// リソースマネージャーの初期化
	bool Init(
	    ComPtr<ID3D12Device> device,
	    DescriptorPool* pools[POOL_COUNT],
	    ComPtr<ID3D12CommandQueue> queue);

	// 終了処理とリソース解放
	void Term();

	// テクスチャの読み込み。キャッシュに存在する場合は再利用する。
	[[nodiscard]] std::shared_ptr<Texture> LoadTexture(
	    const std::wstring& texturePath,
	    bool isSRGB = true);

	// 全てのテクスチャキャッシュを削除する。
	void ClearTextureCache();

	void SetCurrentFenceValue(uint64_t fenceValue);
	void RetireResource(ComPtr<ID3D12Resource>& resource);
	void RetireDescriptor(DescriptorPool* pool, DescriptorHandle*& handle);
	void ReleaseRetiredResources(uint64_t completedFenceValue);
	//----------------------------------------------------------------------
	// Getters / Setters

	[[nodiscard]] ComPtr<ID3D12Device> GetDevice() const { return m_Device; }

	[[nodiscard]] DescriptorPool* GetPool(POOL_TYPE type) const
	{
		if (static_cast<int>(type) >= 0 && static_cast<int>(type) < POOL_COUNT)
		{
			return m_Pool[static_cast<int>(type)];
		}
		return nullptr;
	}

	[[nodiscard]] ComPtr<ID3D12CommandQueue> GetQueue() const { return m_Queue; }

	void SetRenderSystem(RenderSystem* renderSystem) { m_RenderSystem = renderSystem; }
	[[nodiscard]] RenderSystem* GetRenderSystem() const { return m_RenderSystem; }
};
