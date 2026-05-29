#include "Terrain.h"
#include "TerrainScene.h"
#include "Camera.h"
#include "Player.h"
#include "SkyDomeObject.h"
#include "Meadow.h"
#include "Wind.h"
#include "LockOnUI.h"
#include "CombatHud.h"
#include "EnemySpawner.h"
#include "Paladin.h"
#include "PhaseManager.h"
#include "BattleArea.h"
#include "BattleAreaObj.h"
#include "ShopUI.h"
#include "ShopManager.h"
#include "PartyManager.h"
#include "GameMenu.h"
#include "ResultHud.h"
//-----------------------------------------------------------------------------
//	初期化
//-----------------------------------------------------------------------------
void TerrainScene::Init()
{
	// カメラの追加
	auto* pCamera = AddGameObject<Camera>(eLayer::DEFAULT, "Camera");
	if (pCamera)
	{
		XMFLOAT3 pos = { 0.0f, -800.0f, 0.0f };
		pCamera->SetPosition(pos);
		pCamera->SetTarget({ pos.x , pos.y, pos.z + 10.0f });
		SetCamera(pCamera);
	}

	// プレイヤーの追加
	AddGameObject<Player>(eLayer::PLAYER, "Player");

	AddGameObject<PartyManager>(eLayer::DEFAULT, "PartyManager");
	AddGameObject<LockOnUI>(eLayer::UI, "LockOnUI");
	AddGameObject<CombatHud>(eLayer::UI, "CombatHud");

	AddGameObject<GameMenu>(eLayer::MENUUI, "GameMenu");

	// スカイドームの追加
	auto* pSkyDome = AddGameObject<SkyDomeObject>(eLayer::BACKGROUND, "SkyDome");
	if (pSkyDome)
	{
		pSkyDome->SetTexturePath(L"Assets/Texture/Sky/4kHDR");
		pSkyDome->SetRadius(10000.0f);
		pSkyDome->SetSlices(32);
		pSkyDome->SetStacks(16);
	}

	// 地形の追加
	m_Terrain = AddGameObject<Terrain>(eLayer::TERRAIN, "Terrain");
	m_Terrain->Init(
		L"Assets/Texture/Terrain/HeightMap.png",
		L"Assets/Texture/Terrain/FieldMap.png",
		2000.0f, 10.0f);
	m_Terrain->SetPosition({ 0.0f, 0.0f, 0.0f });


	const XMFLOAT3 battleAreaCenter = { -497.0f, 0.0f, -650.0f };
	const float battleAreaRadius = 100.0f;

	BattleArea* battleArea = AddGameObject<BattleArea>(eLayer::DEFAULT, "BattleArea");
	if (battleArea != nullptr)
	{
		battleArea->Setup(battleAreaCenter, battleAreaRadius);
	}
	BattleAreaObj* battleAreaObj = AddGameObject<BattleAreaObj>(eLayer::DEFAULT , "BattleAreaObj");
	if (battleAreaObj != nullptr)
	{
		battleAreaObj->Setup(battleAreaCenter, battleAreaRadius);
	}
	// 風の追加
	AddGameObject<Wind>(eLayer::Environment, "Wind");

	// 草原の追加
	float meadowSize = 0.1f;
	Meadow* pMeadow1 = AddGameObject<Meadow>(eLayer::TERRAIN, "Meadow01");
	pMeadow1->SetModelPath(L"Assets/Model/FieldObject/Grass.bmdl");
	pMeadow1->SetScale(XMFLOAT3(meadowSize, meadowSize, meadowSize));
	pMeadow1->Init(this, m_Terrain);

	AddGameObject<EnemySpawner>(eLayer::DEFAULT, "EnemySpawner");
	AddGameObject<ShopManager>(eLayer::DEFAULT, "ShopManager");
	AddGameObject<ShopUI>(eLayer::UI, "ShopUI");
	AddGameObject<PhaseManager>(eLayer::DEFAULT, "PhaseManager");

	m_ResultHud = AddGameObject<ResultHud>(eLayer::MENUUI, "ResultHud");
}

void TerrainScene::Term()
{
	if (m_Terrain)
	{
		m_Terrain->Term();
		m_Terrain = nullptr;
	}
}

void TerrainScene::Update(float deltaTime)
{
	// リザルト表示中は他の更新を止める
	if (m_ResultHud && m_ResultHud->IsOpen()) {
		m_ResultHud->Update(deltaTime);
		return;
	}

	// メニューが開いている場合、メニュー以外のオブジェクトの更新を停止(deltaTime = 0)する
	const bool menuOpen = GameMenu::IsMenuOpen();
	for (auto& list : m_GameObjectList)
	{
		for (auto& obj : list)
		{
			if (obj->IsDestroyed())
				continue;
			GameMenu* menu = dynamic_cast<GameMenu*>(obj.get());
			const float dt = (menu != nullptr) ? deltaTime : (menuOpen ? 0.0f : deltaTime);
			obj->Update(dt);
		}
	}

	// 破棄予定のオブジェクトをリストから削除
	for (auto& list : m_GameObjectList)
	{
		list.remove_if([](const std::unique_ptr<GameObject>& o) {
			return o->IsDestroyed();
			});
	}

	// 終了判定
	auto* player = GetGameObjectByName<Player>("Player");
	auto* phaseManager = GetGameObjectByName<PhaseManager>("PhaseManager");

	// プレイヤーの死亡アニメーション終了、もしくはフェーズ101到達（100クリア後）
	if ((player && player->IsDeadAnimFinished()) || (phaseManager && phaseManager->IsGameClear()))
	{
		m_ResultHud->Show(); // リザルト演出開始
	}
}
