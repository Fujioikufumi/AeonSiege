#pragma once
#include "SkillData.h"

/// <summary>
/// スキルカタログ：個々のスキルのデータを定義している。
/// ※不変値のため、強化などはm_StatusComponentからの倍率をかける
/// </summary>
class SkillCatalog
{
public:
	static SkillData Create(SkillId id);
};