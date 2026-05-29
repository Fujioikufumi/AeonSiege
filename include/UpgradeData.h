#pragma once
#include <string>

const int kShopOfferCount = 3; // ショップで提示される強化の数
const int kUpgradeKindCount = 11; // 強化内容の種類数
const int kUpgradeRarityCount = 3; // 強化のレアリティ数 (1,2,3)

/// <summary>
/// 強化のレアリティ、数値が高いほど強力な効果を持つ。
/// </summary>
enum class UpgradeRarity
{
	Rarity1 = 1,
	Rarity2 = 2,
	Rarity3 = 3
};

/// <summary>
/// 強化内容の種類
/// </summary>
enum class UpgradeKind
{
	AttackUp,			// 攻撃力上昇
	MoveSpeedUp,		// 移動速度上昇
	DamageReduction,	// 被ダメージ減少
	SkillPowerUp,		// スキルの威力上昇
	SkillCooldownUp,	// スキルのクールダウン減少
	SkillDurationUp,	// スキルの効果時間上昇
	CriticalRateUp,		// クリティカル率上昇
	CriticalDamageUp,   // クリティカルダメージ上昇
	EvasionUp,			// 回避率上昇
	SkillChainDamageUp, // スキルのコンボダメージ上昇
	SkillRangeUp,		// スキルの範囲上昇
};

/// <summary>
/// 強化の効果の種類。
/// </summary>
enum class UpgradeStatType
{
	AttackRate,
	MoveSpeedRate,
	DamageTakenRate,
	MaxHpRate,
	SkillPowerRate,
	SkillCooldownReduction,
	SkillDurationRate,
	CriticalRate,
	CriticalDamageRate,
	EvasionRate,
	SkillChainDamageRate,
	SkillRangeRate,
};

struct UpgradeData
{
	UpgradeKind kind = UpgradeKind::AttackUp;
	UpgradeRarity rarity = UpgradeRarity::Rarity1;
	UpgradeStatType statType = UpgradeStatType::AttackRate;

	float value = 0.0f;

	std::string id;
	std::string name;
	std::string description;

	std::wstring rarityTexturePath;
	std::wstring contentTexturePath;
};