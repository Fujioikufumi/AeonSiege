#pragma once
#include "NameSpace.h"
#include "DescriptorPool.h"
#include <d3d12.h>
#include <ComPtr.h>
#include "SingletonManager.h"

class RenderSystem;

/// <summary>
/// DirectX12 デバイスやデスクリプタプールなどの低レイヤーリソースを中枢管理するシングルトンクラス。
/// </summary>
class ResourceManager : public SingletonManager<ResourceManager>
{
private:
	friend class SingletonManager<ResourceManager>;

	ComPtr<ID3D12Device>       m_Device;
	DescriptorPool* m_Pool[POOL_COUNT] = { nullptr };
	ComPtr<ID3D12CommandQueue> m_Queue;
	RenderSystem* m_RenderSystem = nullptr;

	ResourceManager() = default;
	~ResourceManager() = default;

public:
	/// リソースマネージャーの初期化
	bool Init(
		ComPtr<ID3D12Device> device,
		DescriptorPool* pools[POOL_COUNT],
		ComPtr<ID3D12CommandQueue> queue);

	/// 終了処理とリソース解放
	void Term();

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