#include "GameFlowUtil.h"
#include "GameManager.h"
#include "Scene.h"
#include "ShopManager.h"
#include "GameMenu.h"

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