#pragma once
#include "CombatHudLayout.h"
#include <string>

class GameObject;
class Sprite;
class MaskedSprite;
class NumberUI;


class AllyHudPanel {
public:
	AllyHudPanel(GameObject* owner, const AllyLayout& layout);
	~AllyHudPanel() = default;

	void Update(int hp, int maxHp, int level, float expRatio);

	void SetVisible(bool visible);

	const std::string& GetTargetName() const { return m_Layout.hpFrom; }

	void ApplyLayout(const AllyLayout& newLayout);

private:
	AllyLayout m_Layout; // 味方のレイアウトデータ

	Sprite* m_Panel = nullptr;         // キャラクターパネルの背景
	MaskedSprite* m_HpGray = nullptr;  // HPバーの背景
	MaskedSprite* m_HpCol = nullptr;   // HPバーのカラー部分
	MaskedSprite* m_ExpGray = nullptr; // EXPバーの背景
	MaskedSprite* m_ExpCol = nullptr;  // EXPバーのカラー部分
	NumberUI* m_HpNumber = nullptr;     // HP数値表示
	NumberUI* m_LvNumber = nullptr;     // レベル表示
};