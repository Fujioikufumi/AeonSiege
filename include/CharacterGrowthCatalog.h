#pragma once
#include "PhaseData.h"
#include <string>

class GameObject;

// JSON の base + perLevel（方式A: テーブル積み上げ）で敵ステータスを決定。
// 将来プレイヤー／味方にも同じ形式を流用可能。
namespace CharacterGrowth
{
	bool TryLoadEnemyGrowthTable(const std::wstring& path = L"Assets/AppData/EnemyGrowth.json");
	bool ApplyEnemySpawnStats(GameObject* enemy, EnemyType type, int level);
}