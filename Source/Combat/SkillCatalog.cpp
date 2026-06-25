#include "Combat/SkillCatalog.h"

namespace
{

// スキル1の定数
constexpr float kSkill1PowerRate      = 1.5f;  // スキルの攻撃倍率(StatusComponentのm_Status.attackPowerに掛ける)
constexpr float kSkill1CooldownSec    = 3.0f;  // スキルのクールダウン時間（秒）
constexpr float kSkill1Range          = 50.0f; // スキルの射程距離
constexpr int kSkill1HitCount         = 1;     // スキルのヒット回数（複数回攻撃するスキルの場合）
constexpr float kSkill1EffectDelaySec = 0.75f; // スキル発動後の効果発生までの遅延時間（秒）

// スキル2の定数
constexpr float kSkill2PowerRate      = 1.8f;
constexpr float kSkill2CooldownSec    = 5.0f;
constexpr float kSkill2Range          = 55.0f;
constexpr int kSkill2HitCount         = 2;
constexpr float kSkill2EffectDelaySec = 0.65f;

// スキル3の定数
constexpr float kSkill3PowerRate      = 2.0f;
constexpr float kSkill3CooldownSec    = 4.0f;
constexpr float kSkill3Range          = 55.0f;
constexpr float kSkill3AreaRadius     = 75.0f;
constexpr int kSkill3HitCount         = 1;
constexpr float kSkill3EffectDelaySec = 0.55f;

// スキル4の定数
constexpr float kSkill4PowerRate      = 3.0f;
constexpr float kSkill4CooldownSec    = 10.0f;
constexpr float kSkill4Range          = 90.0f;
constexpr float kSkill4ConeAngleDeg   = 90.0f;
constexpr int kSkill4HitCount         = 1;
constexpr float kSkill4EffectDelaySec = 0.6f;

// スキル5の定数
constexpr float kSkill5PowerRate      = 1.6f;
constexpr float kSkill5CooldownSec    = 7.0f;
constexpr float kSkill5Range          = 65.0f;
constexpr int kSkill5HitCount         = 4;
constexpr float kSkill5HitIntervalSec = 0.3f;
constexpr float kSkill5EffectDelaySec = 0.8f;

// スキル6の定数
constexpr float kSkill6PowerRate      = 1.6f;
constexpr float kSkill6CooldownSec    = 13.0f;
constexpr float kSkill6Range          = 80.0f;
constexpr float kSkill6AreaRadius     = 90.0f;
constexpr int kSkill6HitCount         = 3;
constexpr float kSkill6HitIntervalSec = 1.0f;
constexpr float kSkill6EffectDelaySec = 0.5f;

} // namespace

SkillData SkillCatalog::Create(SkillId id)
{
	SkillData data;
	data.id = id;

	switch (id)
	{
	case SkillId::PlayerSlash1:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::SingleTarget;
		data.animationName  = "Player_Skill01";
		data.skillPowerRate = kSkill1PowerRate;
		data.cooldownSec    = kSkill1CooldownSec;
		data.range          = kSkill1Range;
		data.hitCount       = kSkill1HitCount;
		data.effectDelaySec = kSkill1EffectDelaySec;
		break;

	case SkillId::PlayerSlash2:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::SingleTarget;
		data.animationName  = "Player_Skill02";
		data.skillPowerRate = kSkill2PowerRate;
		data.cooldownSec    = kSkill2CooldownSec;
		data.range          = kSkill2Range;
		data.hitCount       = kSkill2HitCount;
		data.effectDelaySec = kSkill2EffectDelaySec;
		break;

	case SkillId::PlayerSlash3:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::AroundSelf;
		data.animationName  = "Player_Skill03";
		data.skillPowerRate = kSkill3PowerRate;
		data.cooldownSec    = kSkill3CooldownSec;
		data.range          = kSkill3Range;
		data.areaRadius     = kSkill3AreaRadius;
		data.hitCount       = kSkill3HitCount;
		data.effectDelaySec = kSkill3EffectDelaySec;
		break;

	case SkillId::PlayerSlash4:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::ForwardCone;
		data.animationName  = "Player_Skill04";
		data.skillPowerRate = kSkill4PowerRate;
		data.cooldownSec    = kSkill4CooldownSec;
		data.range          = kSkill4Range;
		data.coneAngleDeg   = kSkill4ConeAngleDeg;
		data.hitCount       = kSkill4HitCount;
		data.effectDelaySec = kSkill4EffectDelaySec;
		break;

	case SkillId::PlayerSlash5:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::SingleTarget;
		data.animationName  = "Player_Skill05";
		data.skillPowerRate = kSkill5PowerRate;
		data.cooldownSec    = kSkill5CooldownSec;
		data.range          = kSkill5Range;
		data.hitCount       = kSkill5HitCount;
		data.hitIntervalSec = kSkill5HitIntervalSec;
		data.effectDelaySec = kSkill5EffectDelaySec;
		break;

	case SkillId::PlayerSlash6:
		data.type           = SkillType::Attack;
		data.targetType     = SkillTargetType::AroundTarget;
		data.animationName  = "Player_Skill06";
		data.skillPowerRate = kSkill6PowerRate;
		data.cooldownSec    = kSkill6CooldownSec;
		data.range          = kSkill6Range;
		data.areaRadius     = kSkill6AreaRadius;
		data.hitCount       = kSkill6HitCount;
		data.hitIntervalSec = kSkill6HitIntervalSec;
		data.effectDelaySec = kSkill6EffectDelaySec;
		break;
	default:
		break;
	}

	return data;
}