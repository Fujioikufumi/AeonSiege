#include "EnemySpawner.h"
#include "GameManager.h"
#include "Scene.h"
#include "Terrain.h"
#include "Mutant.h"
#include "Zombie.h"
#include "SkeletonZombie.h"
#include "EnemyAIComponent.h"
#include "CharacterGrowthCatalog.h"
#include "BattleArea.h"
#include "Player.h"
#include <cstdlib>

using namespace DirectX;

EnemySpawner::EnemySpawner()
	: GameObject()
{
}

bool EnemySpawner::Init()
{
	CharacterGrowth::TryLoadEnemyGrowthTable();
	m_TotalSpawnedCount = 0;
	return true;
}

void EnemySpawner::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}

void EnemySpawner::SpawnPhase(const PhaseData& phaseData)
{
	Scene* scene = GameManager::GetScene();
	if (scene == nullptr) return;
	BattleArea* battleArea = scene->GetGameObjectByName<BattleArea>("BattleArea");
	if (battleArea == nullptr)
	{
		ELOG("EnemySpawner: BattleArea not found");
		return;
	}
	Player* player = scene->GetGameObjectByName<Player>("Player");
	if (player == nullptr)
	{
		ELOG("EnemySpawner: Player not found");
		return;
	}
	const XMFLOAT3 center = battleArea->GetCenter();
	const float radius = battleArea->GetRadius();
	const XMFLOAT3 playerPos = player->GetPosition();
	// フェーズ内の総出現数（spread 用）
	int totalCount = 0;
	for (const PhaseEnemyEntry& entry : phaseData.enemies)
		totalCount += entry.count;
	int spawnIndex = 0;
	for (const PhaseEnemyEntry& entry : phaseData.enemies)
	{
		for (int i = 0; i < entry.count; ++i)
		{
			// プレイヤーの位置を考慮してスポーン位置を決定
			// バトルエリアは円などで、エネミーが出現させる座標はプレイヤーから一番離れている円周上に出現させる
			const XMFLOAT3 spawnPosition = CreateSpawnPosition(
				center, radius, playerPos, spawnIndex, totalCount);
			GameObject* enemy = SpawnEnemy(
				entry.type, spawnPosition, m_TotalSpawnedCount, entry.level, entry.expReward);
			if (enemy != nullptr)
				++m_TotalSpawnedCount;
			++spawnIndex;
		}
	}
}

XMFLOAT3 EnemySpawner::CreateRandomSpawnPosition(const XMFLOAT3& center, float radius) const
{
	Scene* pScene = GameManager::GetScene();
	Terrain* pTerrain = (pScene != nullptr) ? pScene->GetGameObjectByName<Terrain>("Terrain") : nullptr;

	const float angle = (rand() % 360) * (XM_PI / 180.0f);
	const float distance = (rand() % 100 / 100.0f) * radius;

	XMFLOAT3 position = center;
	position.x += cosf(angle) * distance;
	position.z += sinf(angle) * distance;

	if (pTerrain != nullptr)
	{
		position.y = pTerrain->GetHeightAt(position.x, position.z);
	}

	return position;
}

DirectX::XMFLOAT3 EnemySpawner::CreateSpawnPosition(const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT3& playerPos, int spreadIndex, int spreadCount) const
{
	const float dx = playerPos.x - center.x;
	const float dz = playerPos.z - center.z;
	const float distSq = dx * dx + dz * dz;
	float angle = 0.0f;

	// プレイヤーが中心とほぼ一致する場合はランダムな境界上
	if (distSq < 0.0001f)
	{
		angle = (rand() % 360) * (XM_PI / 180.0f);
	}
	else
	{
		// centerからplayerPosの逆方向の角度を計算
		const float baseAngle = atan2f(-dz, -dx);
		// 複数体の重なり防止
		const float spreadStep = 0.45f; // 一定角度ずらす
		const float offset = (spreadCount <= 1)
			? 0.0f
			: (static_cast<float>(spreadIndex) - (spreadCount - 1) * 0.5f) * spreadStep;
		angle = baseAngle + offset;
	}

	XMFLOAT3 position{};
	position.x = center.x + cosf(angle) * radius;
	position.z = center.z + sinf(angle) * radius;

	// 地形の高さに合わせる
	Scene* scene = GameManager::GetScene();
	Terrain* terrain = scene->GetGameObjectByName<Terrain>("Terrain");
	position.y = terrain->GetHeightAt(position.x, position.z);

	return position;
}

GameObject* EnemySpawner::SpawnEnemy(EnemyType type, const XMFLOAT3& position, int index, int level, int expReward)
{
	Scene* pScene = GameManager::GetScene();
	if (pScene == nullptr) return nullptr;

	GameObject* enemy = nullptr;

	// 各種類のエネミーの出現
	switch (type)
	{
	case EnemyType::Zombie:
		enemy = pScene->AddGameObject<Zombie>(eLayer::ENEMY, "Zombie" + std::to_string(index + 1));
		break;

	case EnemyType::SkeletonZombie:
		enemy = pScene->AddGameObject<SkeletonZombie>(eLayer::ENEMY, "SkeletonZombie" + std::to_string(index + 1));
		break;

	case EnemyType::Mutant:
		enemy = pScene->AddGameObject<Mutant>(eLayer::ENEMY, "Mutant" + std::to_string(index + 1));
		break;
	}

	if (enemy == nullptr) return nullptr;

	enemy->SetIsCulled(true);
	enemy->SetPosition(position);

	EnemyAIComponent* ai = enemy->GetComponent<EnemyAIComponent>();
	if (ai != nullptr)
	{
		ai->SetSpawnPosition(position);
		ai->SetExpReward(expReward);
	}

	CharacterGrowth::ApplyEnemySpawnStats(enemy, type, level);

	return enemy;
}