#include "World/Terrain.h"
#include "World/TerrainScene.h"
#include "Input/Camera.h"
#include "Character/Player.h"
#include "World/SkyDomeObject.h"
#include "World/Meadow.h"
#include "World/Wind.h"
#include "UI/LockOnUI.h"
#include "UI/HUD/CombatHud.h"
#include "Character/EnemySpawner.h"
#include "Character/Paladin.h"
#include "Flow/PhaseManager.h"
#include "World/BattleArea.h"
#include "World/BattleAreaObj.h"
#include "UI/ShopUI.h"
#include "Flow/ShopManager.h"
#include "Character/PartyManager.h"
#include "UI/GameMenu.h"
#include "UI/ResultHud.h"

namespace {
	// カメラの定数
	constexpr XMFLOAT3 kCameraStartPos = { 0.0f, -800.0f, 0.0f }; // カメラの初期位置

	// スカイドームの定数
	constexpr float kSkyDomeRadius = 10000.0f; // スカイドームの半径
	constexpr int kSkyDomeSlices = 32; // スカイドームのスライス数
	constexpr int kSkyDomeStacks = 16; // スカイドームのスタック数

	// 地形の定数
	constexpr float kHeightScale = 2000.0f; // 地形の高さのスケール
	constexpr float kGridSize = 10.0f; // 地形のグリッドサイズ
	constexpr XMFLOAT3 kTerrainCenter = { 0.0f, 0.0f, 0.0f }; // 地形の中心座標

	// バトルエリアの定数
	constexpr float kBattleAreaRadius = 100.0f; // バトルエリアの半径
	constexpr XMFLOAT3 kBattleAreaCenter = { -497.0f, 0.0f, -650.0f }; // バトルエリアの中心座標

	// 草原の定数
	constexpr float kMeadowSize = 0.1f; // 草原のスケール
}

//-----------------------------------------------------------------------------
//	初期化
//-----------------------------------------------------------------------------
void TerrainScene::Init()
{
	// カメラの追加
	Camera* pCamera = AddGameObject<Camera>(eLayer::DEFAULT, "Camera");
	if (pCamera)
	{
		XMFLOAT3 pos = kCameraStartPos;
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
	SkyDomeObject* pSkyDome = AddGameObject<SkyDomeObject>(eLayer::BACKGROUND, "SkyDome");
	if (pSkyDome)
	{
		pSkyDome->SetTexturePath(L"Assets/Texture/Sky/4kHDR");
		pSkyDome->SetRadius(kSkyDomeRadius);
		pSkyDome->SetSlices(kSkyDomeSlices);
		pSkyDome->SetStacks(kSkyDomeStacks);
	}

	// 地形の追加
	m_Terrain = AddGameObject<Terrain>(eLayer::TERRAIN, "Terrain");
	m_Terrain->Init(
		L"Assets/Texture/Terrain/HeightMap.png",
		L"Assets/Texture/Terrain/FieldMap.png",
		kHeightScale, kGridSize);
	m_Terrain->SetPosition(kTerrainCenter);



	BattleArea* battleArea = AddGameObject<BattleArea>(eLayer::DEFAULT, "BattleArea");
	if (battleArea != nullptr)
	{
		battleArea->Setup(kBattleAreaCenter, kBattleAreaRadius);
	}
	BattleAreaObj* battleAreaObj = AddGameObject<BattleAreaObj>(eLayer::DEFAULT , "BattleAreaObj");
	if (battleAreaObj != nullptr)
	{
		battleAreaObj->Setup(kBattleAreaCenter, kBattleAreaRadius);
	}
	// 風の追加
	AddGameObject<Wind>(eLayer::Environment, "Wind");

	// 草原の追加
	float meadowSize = kMeadowSize;
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
