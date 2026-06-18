#include "Flow/GameFlowUtil.h"
#include "Core/GameManager.h"
#include "Core/Scene.h"
#include "Flow/ShopManager.h"
#include "UI/GameMenu.h"

namespace GameFlowUtil
{
	bool IsShopOpen()
	{
		Scene* scene = GameManager::GetScene();
		if (scene == nullptr)
		{
			return false;
		}

		ShopManager* shopManager = scene->GetGameObjectByName<ShopManager>("ShopManager");
		return shopManager != nullptr && shopManager->IsOpen();
	}

	bool IsMenuOpen()
	{
		return GameMenu::IsMenuOpen();
	}
}