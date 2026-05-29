#include "SkillCatalog.h"

SkillData SkillCatalog::Create(SkillId id)
{
	SkillData data;
	data.id = id;

	switch (id)
	{
	case SkillId::PlayerSlash1:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::SingleTarget;
		data.animationName = "Player_Skill01";
		data.skillPowerRate = 1.5f;
		data.cooldownSec = 3.0f;
		data.range = 50.0f;
		data.hitCount = 1;
		data.effectDelaySec = 0.75f;
		break;

	case SkillId::PlayerSlash2:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::SingleTarget;
		data.animationName = "Player_Skill02";
		data.skillPowerRate = 1.8f;
		data.cooldownSec = 5.0f;
		data.range = 55.0f;
		data.hitCount = 1;
		data.effectDelaySec = 0.65f;
		break;

	case SkillId::PlayerSlash3:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::AroundSelf;
		data.animationName = "Player_Skill03";
		data.skillPowerRate = 2.0f;
		data.cooldownSec = 4.0f;
		data.range = 55.0f;
		data.areaRadius = 75.0f;
		data.hitCount = 1;
		data.effectDelaySec = 0.55f;
		break;

	case SkillId::PlayerSlash4:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::ForwardCone;
		data.animationName = "Player_Skill04";
		data.skillPowerRate = 3.0f;
		data.cooldownSec = 10.0f;
		data.range = 90.0f;
		data.coneAngleDeg = 90.0f;
		data.hitCount = 1;
		data.effectDelaySec = 0.6f;
		break;

	case SkillId::PlayerSlash5:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::SingleTarget;
		data.animationName = "Player_Skill05";
		data.skillPowerRate = 1.6f;
		data.cooldownSec = 7.0f;
		data.range = 65.0f;
		data.hitCount = 4;
		data.hitIntervalSec = 0.3f;
		data.effectDelaySec = 0.8f;
		break;

	case SkillId::PlayerSlash6:
		data.type = SkillType::Attack;
		data.targetType = SkillTargetType::AroundTarget;
		data.animationName = "Player_Skill06";
		data.skillPowerRate = 1.6f;
		data.cooldownSec = 13.0f;
		data.range = 80.0f;
		data.areaRadius = 90.0f;
		data.hitCount = 3;
		data.hitIntervalSec = 1.0f;
		data.effectDelaySec = 0.5f;
		break;
	default:
		break;
	}

	return data;
}