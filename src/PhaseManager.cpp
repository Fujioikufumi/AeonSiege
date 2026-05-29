#include "PhaseManager.h"
#include "EnemySpawner.h"
#include "GameManager.h"
#include "Scene.h"
#include "HealthComponent.h"
#include "ShopManager.h"
#include "FileUtil.h"
#include "EncodingUtils.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

namespace {

	static constexpr wchar_t kPhasesJsonPath[] = L"Assets/AppData/Phases.json";

	// JSON の type 文字列を EnemyType に変換する
	static bool ParseEnemyTypeString(const std::string& key, EnemyType& outType)
	{
		if (key == "Zombie") { outType = EnemyType::Zombie; return true; }
		if (key == "SkeletonZombie") { outType = EnemyType::SkeletonZombie; return true; }
		if (key == "Mutant") { outType = EnemyType::Mutant; return true; }
		return false;
	}

	// Jsonに書かれたフェーズの情報を PhaseData に変換する
	static PhaseData ParsePhaseJson(const nlohmann::json& jp, int defaultMaxSpawn)
	{
		PhaseData p{};

		// フェーズ番号を取得。指定がない場合は1とする
		p.phaseNo = jp.value("phaseNo", 1);

		int remaining = max(0, defaultMaxSpawn);

		// enemies 配列を取得。存在しない、または配列でない場合は nullptr とする
		const nlohmann::json* enemiesJson = nullptr;
		if (jp.contains("enemies") && jp["enemies"].is_array())
			enemiesJson = &jp["enemies"];

		if (enemiesJson != nullptr)
		{
			for (const auto& je : *enemiesJson)
			{
				if (!je.contains("type") || !je["type"].is_string())
					continue;

				EnemyType t{};
				if (!ParseEnemyTypeString(je["type"].get<std::string>(), t))
				{
					ELOG("Phases.json: unknown enemy type '%s'", je["type"].get<std::string>().c_str());
					continue;
				}

				int count = je.value("count", 1);
				if (count <= 0)
					continue;
				// maxEnemiesPerPhase を超えないよう、フェーズ内の合計出現数で打ち切り
				count = min(count, remaining);
				if (count <= 0)
					break;

				// フェーズ内のエネミー情報を作成
				PhaseEnemyEntry e{};
				e.type = t;
				e.count = count;
				e.level = je.value("level", 1);
				e.expReward = je.value("expReward", 0);

				p.enemies.push_back(e);
				remaining -= count;
				if (remaining <= 0)
					break;
			}
		}

		return p;
	}

} // namespace

void PhaseManager::LoadPhaseData()
{
	m_Phases.clear();

	// ファイルパスの検索
	std::wstring fullPath;
	if (!SearchFilePathW(kPhasesJsonPath, fullPath))
	{
		ELOG("Phases: file not found, using built-in test phases");
		return;
	}

	// JSONファイルの読み込み
	std::ifstream ifs(EncodeWideToUtf8(fullPath));
	if (!ifs)
	{
		ELOG("Phases: failed to open: %s", EncodeWideToUtf8(fullPath).c_str());
		return;
	}

	// nlohmann::json でのパース
	nlohmann::json root;
	try
	{
		ifs >> root;
	}
	catch (const std::exception& ex)
	{
		ELOG("Phases JSON parse error: %s", ex.what());
		return;
	}

	int maxPerPhase = 6;
	if (root.contains("maxEnemiesPerPhase"))
		maxPerPhase = root["maxEnemiesPerPhase"].get<int>();

	if (!root.contains("phases") || !root["phases"].is_array())
	{
		ELOG("Phases: missing 'phases' array");
		return;
	}

	m_Phases.reserve(root["phases"].size());
	for (const auto& jp : root["phases"])
		m_Phases.push_back(ParsePhaseJson(jp, maxPerPhase));
}

PhaseManager::PhaseManager()
	: GameObject()
{
}

bool PhaseManager::Init()
{
	if (!GameObject::Init())
		return false;

	LoadPhaseData();

	Scene* scene = GameManager::GetScene();
	if (scene != nullptr)
	{
		m_EnemySpawner = scene->GetGameObjectByName<EnemySpawner>("EnemySpawner");
		m_ShopManager = scene->GetGameObjectByName<ShopManager>("ShopManager");
	}

	m_State = PhaseState::Spawning;
	m_CurrentPhaseIndex = 0; // ゲーム開始時のフェーズ番号
	m_PhaseClearTimer = 0.0f;

	BeginSpawnPhase();

	return true;
}

void PhaseManager::Update(float deltaTime)
{
	switch (m_State)
	{
	case PhaseState::Idle:
		break;

	case PhaseState::Spawning:
		// エネミーのスポーンはフェーズ開始から少し遅らせる
		m_SpawnDelayTimer -= deltaTime;
		if (m_SpawnDelayTimer > 0.0f)
			break;

		StartCurrentPhase();
		m_State = PhaseState::Fighting;
		m_SpawnDelayTimer = 0.0f;
		break;

	case PhaseState::Fighting:
		if (AreAllEnemiesDefeated())
		{
			m_PhaseClearTimer = kPhaseClearWaitSec;
			m_State = PhaseState::PhaseClear;
		}
		break;

	case PhaseState::PhaseClear:
		m_PhaseClearTimer -= deltaTime;
		if (m_PhaseClearTimer <= 0.0f)
		{
			AdvanceToNextPhase();
		}
		break;

	case PhaseState::Shopping:
		UpdateShopping(deltaTime);
		break;

	case PhaseState::GameClear:
		break;
	}

	GameObject::Update(deltaTime);
}

int PhaseManager::GetCurrentPhaseNumber() const
{
	if (m_Phases.empty())
		return 0;

	// インデックスが範囲外の場合は最終フェーズの番号を返す
	if (m_CurrentPhaseIndex < 0 ||
		m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
	{
		return m_Phases.back().phaseNo;
	}

	return m_Phases[m_CurrentPhaseIndex].phaseNo;
}

void PhaseManager::StartCurrentPhase()
{
	if (m_EnemySpawner == nullptr)
	{
		Scene* scene = GameManager::GetScene();
		m_EnemySpawner = (scene != nullptr) ? scene->GetGameObjectByName<EnemySpawner>("EnemySpawner") : nullptr;
	}

	if (m_EnemySpawner == nullptr) return;

	if (m_CurrentPhaseIndex < 0 || m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
	{
		// フェーズデータが存在しない場合はゲームクリアとする
		m_State = PhaseState::GameClear;
		return;
	}

	// 現在のフェーズ番号をゲームステータスに反映
	GameManager::GetStatus().currentPhase = m_Phases[m_CurrentPhaseIndex].phaseNo;

	// エネミーを出現させる
	m_EnemySpawner->SpawnPhase(m_Phases[m_CurrentPhaseIndex]);
}

bool PhaseManager::AreAllEnemiesDefeated() const
{
	Scene* scene = GameManager::GetScene();
	if (scene == nullptr) return false;

	// シーン内にeLayer::ENEMYのゲームオブジェクトが存在しない場合は、全て倒されたとみなす
	const auto& enemies = scene->GetGameObjectsByLayer(eLayer::ENEMY);

	for (const auto& enemy : enemies)
	{
		if (enemy == nullptr || enemy->IsDestroyed())
		{
			continue;
		}

		HealthComponent* hp = enemy->GetComponent<HealthComponent>();
		if (hp != nullptr && hp->IsAlive())
		{
			return false;
		}
	}

	return true;
}

void PhaseManager::AdvanceToNextPhase()
{
	if (ShouldOpenShop())
	{
		StartShopping();
		return;
	}

	++m_CurrentPhaseIndex;

	if (m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
	{
		m_State = PhaseState::GameClear;
		return;
	}

	BeginSpawnPhase();
}

bool PhaseManager::ShouldOpenShop() const
{
	if (m_CurrentPhaseIndex < 0 || m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
	{
		return false;
	}
	const int clearedPhaseNo = m_Phases[m_CurrentPhaseIndex].phaseNo;

	// フェーズ1終了後、または指定の間隔（5フェーズごと）にショップを開く
	return clearedPhaseNo == 1 || (clearedPhaseNo > 0 && (clearedPhaseNo % kShopIntervalPhase) == 0);
}

void PhaseManager::BeginSpawnPhase()
{
	m_State = PhaseState::Spawning;
	m_SpawnDelayTimer = kSpawnDelaySec;
}


void PhaseManager::StartShopping()
{
	m_HasOpenedShop = false;
	m_State = PhaseState::Shopping;
}


void PhaseManager::UpdateShopping(float deltaTime)
{

	if (m_ShopManager == nullptr)
	{
		Scene* scene = GameManager::GetScene();
		m_ShopManager = (scene != nullptr)
			? scene->GetGameObjectByName<ShopManager>("ShopManager")
			: nullptr;
	}
	if (m_ShopManager == nullptr)
	{
		++m_CurrentPhaseIndex;
		if (m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
		{
			m_State = PhaseState::GameClear;
		}
		else
		{
			BeginSpawnPhase();
		}
		return;
	}
	if (!m_HasOpenedShop)
	{
		m_ShopManager->OpenShop(m_Phases[m_CurrentPhaseIndex].phaseNo);
		m_HasOpenedShop = true;
		return;
	}
	if (m_ShopManager->IsOpen())
	{
		return;
	}
	++m_CurrentPhaseIndex;
	if (m_CurrentPhaseIndex >= static_cast<int>(m_Phases.size()))
	{
		m_State = PhaseState::GameClear;
		return;
	}
	BeginSpawnPhase();
}