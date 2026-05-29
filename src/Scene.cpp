#include "Scene.h"

void Scene::Init()
{
}

void Scene::Term()
{
	// オブジェクトの生ポインタキャッシュをクリア
	m_NameCache.clear();
	m_PointerCache.clear();

	// 実体を破棄
	for (auto& list : m_GameObjectList)
	{
		list.clear();
	}
}

void Scene::Update(float deltaTime)
{
	// 1. 全オブジェクトの更新
	for (auto& list : m_GameObjectList)
	{
		for (auto& obj : list)
		{
			if (!obj->IsDestroyed())
			{
				obj->Update(deltaTime);
			}
		}
	}

	// 2. 破棄フラグ(IsDestroyed)が立っているオブジェクトのクリーンアップ
	for (auto& list : m_GameObjectList)
	{
		list.remove_if([this](const std::unique_ptr<GameObject>& obj) {
			if (obj->IsDestroyed())
			{
				// リストから消える前に、キャッシュ（生ポインタ）からも削除する
				GameObject* rawPtr = obj.get();

				// セットキャッシュから削除
				m_PointerCache.erase(rawPtr);

				// 名前キャッシュからも削除（名前が空でなければ）
				const std::string& name = rawPtr->GetObjName();
				if (!name.empty())
				{
					// 万が一、別の新しい同名オブジェクトが登録されて上書きされていないか確認して消す
					auto it = m_NameCache.find(name);
					if (it != m_NameCache.end() && it->second == rawPtr)
					{
						m_NameCache.erase(it);
					}
				}

				return true; // remove_if によってリストから削除される
			}
			return false;
			});
	}
}

void Scene::Draw(const RenderContext& context)
{
	for (auto& list : m_GameObjectList)
	{
		for (auto& obj : list)
		{
			// 破棄フラグが立っているものは描画しない
			if (!obj->IsDestroyed())
			{
				obj->Draw(context);
			}
		}
	}
}

void Scene::RemoveGameObject(GameObject* obj)
{
	if (obj != nullptr)
	{
		// 存在すれば削除フラグを立てる
		obj->Destroy();
	}
}

bool Scene::ContainsGameObject(const GameObject* obj) const
{
	if (obj == nullptr)
		return false;

	return m_PointerCache.contains(obj);
}