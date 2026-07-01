#include "Core/Scene.h"

void Scene::Init()
{
}

void Scene::Term()
{
	m_NameCache.clear();
	m_PointerCache.clear();

	for (auto& list : m_GameObjectList)
		list.clear();

	m_PendingDeletion.clear();
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

	for (auto& list : m_GameObjectList)
	{
		for (auto it = list.begin(); it != list.end();)
		{
			if ((*it)->IsDestroyed())
			{
				GameObject* rawPtr = it->get();
				m_PointerCache.erase(rawPtr);

				const std::string& name = rawPtr->GetObjName();
				if (!name.empty())
				{
					auto nit = m_NameCache.find(name);
					if (nit != m_NameCache.end() && nit->second == rawPtr)
						m_NameCache.erase(nit);
				}

				// GPUがまだ参照している可能性があるため、フェンス完了まで実体破棄しない
				m_PendingDeletion.push_back({std::move(*it), 0});
				it = list.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// 遅延削除キューの寿命を進め、0になったものを実際に解放する
	//for (auto it = m_PendingDeletion.begin(); it != m_PendingDeletion.end();)
	//{
	//	if (it->second == 0)
	//		it = m_PendingDeletion.erase(it); // ここで実体破棄＝GPUリソース解放
	//	else
	//	{
	//		--it->second;
	//		++it;
	//	}
	//}
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

void Scene::SetPendingDeletionFenceValue(uint64_t fenceValue)
{
	for (auto& pending : m_PendingDeletion)
	{
		if (pending.FenceValue == 0)
		{
			pending.FenceValue = fenceValue;
		}
	}
}

void Scene::ReleasePendingDeletion(uint64_t completedFenceValue)
{
	for (auto it = m_PendingDeletion.begin(); it != m_PendingDeletion.end();)
	{
		if (it->FenceValue != 0 && it->FenceValue <= completedFenceValue)
		{
			it = m_PendingDeletion.erase(it);
		}
		else
		{
			++it;
		}
	}
}
