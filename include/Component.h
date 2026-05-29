#pragma once
#include "ResourceManager.h"
#include "RenderContext.h"
#include <nlohmann/json.hpp>
#include <string>

class GameObject;

/// <summary>
/// GameObjectにアタッチ可能なコンポーネントクラス。
/// </summary>
class Component
{
protected:
	GameObject* m_GameObject = nullptr;			 // 所有者
	std::string m_ComponentName = "Component";   // コンポーネント名

	friend class GameObject;

public:
	Component() = delete;
	Component(GameObject* obj) : m_GameObject(obj) {}

	virtual ~Component() = default;

	//---------------------------------------------------------
	// 基本インターフェース

	virtual bool Init() { return true; }
	virtual void Term() {}

	[[nodiscard]] virtual const std::string& GetComponentName() const { return m_ComponentName; }

	// デバッグ用ウィンドウ（派生クラスでUI実装）
	virtual void OnDebugWindow() {}

	//---------------------------------------------------------
	// シリアライズ処理

	/// 保存用のJsonデータを構築します
	virtual void Serialize(nlohmann::json& json) const {}

	/// Jsonデータからコンポーネントの状態を復元します
	virtual void Deserialize(const nlohmann::json& json) {}

protected:
	virtual void Update(float deltaTime) {}
	virtual void Draw(const RenderContext& context) {}

	//---------------------------------------------------------
	// ユーティリティ (ResourceManager へのアクセス)

	[[nodiscard]] ComPtr<ID3D12Device> GetDevice() const
	{
		return ResourceManager::GetInstance().GetDevice();
	}

	[[nodiscard]] DescriptorPool* GetPool(POOL_TYPE type) const
	{
		return ResourceManager::GetInstance().GetPool(type);
	}

	[[nodiscard]] ComPtr<ID3D12CommandQueue> GetQueue() const
	{
		return ResourceManager::GetInstance().GetQueue();
	}

	[[nodiscard]] GameObject* GetGameObject() const { return m_GameObject; }
};