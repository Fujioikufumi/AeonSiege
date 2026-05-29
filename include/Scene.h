#pragma once
#include <list>
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "GameObject.h"
#include "d3d12.h"


class Camera;

class Scene
{
protected:
	// レイヤーごとにオブジェクトを管理するリスト
	std::array<std::list<std::unique_ptr<GameObject>>, (int)eLayer::MAX> m_GameObjectList;
	// オブジェクト検索用のキャッシュ
	std::unordered_map<std::string, GameObject*> m_NameCache;
	// nullptr チェック用のポインタ
	std::unordered_set<const GameObject*> m_PointerCache;

	// カメラはシーンにも直接持たせる (GetGameObjectでも取得可能)
	Camera* m_Camera = nullptr;
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void Init();
	virtual void Term();
	virtual void Update(float deltaTime);
	virtual void Draw(const RenderContext& context);

	[[nodiscard]] Camera* GetCamera() const { return m_Camera; }
	void SetCamera(Camera* camera) { m_Camera = camera; }

	// 指定したレイヤー全てのオブジェクトを取得
	[[nodiscard]] const std::list<std::unique_ptr<GameObject>>& GetGameObjectsByLayer(eLayer layer) const
	{
		return m_GameObjectList[(int)layer];
	}

	// 指定されたポインタのオブジェクトの削除
	void RemoveGameObject(GameObject* obj);

	// 現在のシーンにオブジェクトを追加
	template <typename T = GameObject>
	T* AddGameObject(eLayer layer = eLayer::DEFAULT, const std::string& name = "")
	{
		auto gameObj = std::make_unique<T>();
		T* rawPtr = gameObj.get(); // 生ポインタをキャッシュ用に保持
		rawPtr->SetLayer(layer);

		// キャッシュに登録
		if (!name.empty())
		{
			m_NameCache[name] = rawPtr;
		}
		m_PointerCache.insert(rawPtr);

		m_GameObjectList[(int)layer].push_back(std::move(gameObj));
		
		rawPtr->Init();
		rawPtr->SetObjName(name);

		return rawPtr;
	}

	// AddGameObjectで登録した名前でオブジェクトを取得
	template <typename T = GameObject>
	[[nodiscard]] T* GetGameObjectByName(const std::string& name) const
	{
		// ハッシュマップでO(1)の高速検索
		auto it = m_NameCache.find(name);
		if (it != m_NameCache.end())
		{
			// 毎フレーム実行するとパフォーマンスに影響が出る可能性がある
			return dynamic_cast<T*>(it->second);
		}
		return nullptr;
	}

	// AddGameObjectで登録したオブジェクトの型で全検索
	template <typename T = GameObject>
	[[nodiscard]] T* GetGameObject() const
	{
		// 全検索
		for (const auto& list : m_GameObjectList)
		{
			for (const auto& obj : list)
			{
				T* ret = dynamic_cast<T*>(obj.get());
				if (ret != nullptr)
				{
					return ret;
				}
			}
		}
		return nullptr;
	}

	// 全てのオブジェクトを生ポインタのベクターで取得
	[[nodiscard]] std::vector<GameObject*> GetAllGameObjects() const
	{
		std::vector<GameObject*> allObjects;
		allObjects.reserve(m_PointerCache.size());

		for (const auto& list : m_GameObjectList)
		{
			for (const auto& obj : list)
			{
				if (obj) allObjects.push_back(obj.get());
			}
		}
		return allObjects;
	}

	// 指定されたオブジェクトがシーン内に存在するか
	[[nodiscard]] bool ContainsGameObject(const GameObject* obj) const;
};