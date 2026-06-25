#include "UI/HUD/PhaseHudPanel.h"
#include "Core/GameObject.h"
#include "Graphics/Sprite/Sprite.h"
#include "UI/NumberUI.h"
#include "UI/HUD/CombatHudLayout.h"
#include "Core/GameManager.h"
#include "Core/Scene.h"
#include "Core/NameSpace.h"

PhaseHudPanel::PhaseHudPanel(GameObject* owner)
{
	// Phase ラベル（CombatHud の Component として所有）
	m_Label = owner->AddComponent<Sprite>();
	m_Label->Init(kPhaseLabelTex);

	// 数字（NumberUI は GameObject のため Scene 登録）
	Scene* scene = GameManager::GetScene();
	m_Number     = scene->AddGameObject<NumberUI>(eLayer::UI, "CombatHudPhaseNumber");
	m_Number->Init();
	m_Number->SetTexturePath(kNumberTex);
	ApplyLayout();
}

void PhaseHudPanel::ApplyLayout()
{
	if (m_Label)
	{
		m_Label->SetSize(
		    HudLayoutUtil::ScreenWidthRatio(kLabelWidth),
		    HudLayoutUtil::ScreenHeightRatio(kLabelHeight));
		m_Label->SetPosition(
		    HudLayoutUtil::ScreenWidthRatio(kLabelCenterX),
		    HudLayoutUtil::ScreenHeightRatio(kLabelCenterY));
	}

	if (m_Number)
	{
		m_Number->SetPosition(
		    HudLayoutUtil::ScreenWidthRatio(kNumberOnesX),
		    HudLayoutUtil::ScreenHeightRatio(kNumberY));
		m_Number->SetScale(kNumberScale);
	}
}

void PhaseHudPanel::Update(int phaseNo)
{
	if (m_Number)
		m_Number->SetValue(phaseNo);
}
