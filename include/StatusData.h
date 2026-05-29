#pragma once

/// <summary>
/// キャラクターのステータスデータ
/// </summary>
struct StatusData
{
	// 基本値
	int   maxHp = 100;
	int   attackPower = 10;
	int   defensePower = 5;
	float moveSpeed = 30.0f;
	float autoAttackInterval = 1.0f;

	// 戦闘補正
	float criticalRate = 0.0f;
	float criticalDamageRate = 1.5f;
	float evasionRate = 0.0f;
	float skillChainDamageRate = 1.25f;

	// スキル補正
	float skillRangeRate = 1.0f;
	float skillCooldownRate = 1.0f;
	float skillPowerRate = 1.0f;
	float skillDurationRate = 1.0f;

	// 最終倍率
	float attackRate = 1.0f;
	float maxHpRate = 1.0f;
	float moveSpeedRate = 1.0f;
	float damageTakenRate = 1.0f;
};