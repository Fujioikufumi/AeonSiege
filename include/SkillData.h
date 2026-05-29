#pragma once

/// <summary>
/// スキルID
/// </summary>
enum class SkillId
{
	None,
	PlayerSlash1,
	PlayerSlash2,
	PlayerSlash3,
	PlayerSlash4,
	PlayerSlash5,
	PlayerSlash6
};

/// <summary>
/// スキルの種類
/// </summary>
enum class SkillType
{
	Attack,
	Heal,
	Guard,
	Buff
};

/// <summary>
/// スキルの対象範囲の種類
/// </summary>
enum class SkillTargetType
{
	SingleTarget, // 単体
	AroundSelf,  // 自身の周囲
	AroundTarget, // ターゲットの周囲
	ForwardCone  // 前方扇形
};

/// <summary>
/// スキルのデータ構造体
/// </summary>
struct SkillData
{
	SkillId id = SkillId::None; // スキルID
	SkillType type = SkillType::Attack; // スキルの種類
	SkillTargetType targetType = SkillTargetType::SingleTarget; // スキルの対象範囲の種類

	const char* animationName = nullptr; // スキル発動時のアニメーション名

	float skillPowerRate = 0.0f;	// スキルの攻撃倍率(StatusComponentのm_Status.attackPowerに掛ける)
	float cooldownSec = 3.0f;		// スキルのクールダウン時間（秒）
	float range = 0.0f;				// スキルの射程距離
	int hitCount = 1;				// スキルのヒット回数（複数回攻撃するスキルの場合）
	float hitIntervalSec = 0.0f;	// 複数ヒットする場合のヒット感覚(秒) ※一定間隔前提
	float effectDelaySec = 0.0f;	// スキル発動後の効果発生までの遅延時間（秒）
	float areaRadius = 0.0f;		// スキルの範囲半径（円形範囲の場合）
	float coneAngleDeg = 0.0f;		// スキルの扇形角度（前方扇形の場合）
	float durationSec = 0.0f;		// スキルの持続時間（秒）
};