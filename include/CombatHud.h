#pragma once
#include "GameObject.h"
#include "CombatHudLayout.h"
#include "AllyHudPanel.h"
#include "EnemyHudPanel.h"
#include "SkillHudPanel.h"
#include "PhaseHudPanel.h"
#include <vector>
#include <memory>

class CombatHud : public GameObject 
{
public:
	CombatHud() = default;
	~CombatHud() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	// DebugUI用アクセス
	CombatHudLayout& GetLayout() { return m_LayoutData; }
	void RefreshLayout();
private:

	CombatHudLayout m_LayoutData; // HUD全体のレイアウトデータ

	std::unique_ptr<EnemyHudPanel> m_EnemyPanel; // 敵のHUDパネル
	std::vector<std::unique_ptr<AllyHudPanel>> m_AllyPanels;    // 味方のHUDパネル
	std::vector<std::unique_ptr<SkillHudPanel>> m_SkillPanels;  // スキルのHUDパネル
	std::unique_ptr<PhaseHudPanel> m_PhasePanel; // フェーズ表示パネル
};