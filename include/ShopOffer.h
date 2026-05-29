#pragma once
#include "UpgradeData.h"
#include "SkillData.h"
#include <string>

enum class ShopOfferType
{
	StatusUpgrade, // ステータス強化
	JoinAlly,	   // 仲間加入
	UnlockSkill	   // スキル解放
};

enum class AllyId
{
	None, 
	Oscar // オスカー(Paladin)
};

struct ShopOffer
{
	ShopOfferType type = ShopOfferType::StatusUpgrade;

	UpgradeData upgrade;
	AllyId allyId = AllyId::None;
	SkillId skillId = SkillId::None;

	std::wstring rarityTexturePath;
	std::wstring contentTexturePath;
};