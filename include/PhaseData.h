#pragma once

#include <DirectXMath.h>
#include <vector>

/// <summary>
/// 出現するエネミーの種類
/// </summary>
enum class EnemyType
{
	Zombie,
	SkeletonZombie,
	Mutant
};



/// <summary>
/// フェーズごとのエネミー出現情報
/// </summary>
struct PhaseEnemyEntry
{
	EnemyType type = EnemyType::Zombie;
	int count = 0;
	int level = 1;       // スポーン時のレベル（方式Aテーブルで Status を算出）
	int expReward = 0;   // 撃破時に付与する経験値
};

/// <summary>
/// フェーズの情報(フェーズ番号、出現するエネミーの情報)
/// </summary>
struct PhaseData
{
	int phaseNo = 1;
	std::vector<PhaseEnemyEntry> enemies;
};