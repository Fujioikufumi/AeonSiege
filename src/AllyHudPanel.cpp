#include "AllyHudPanel.h"
#include "GameObject.h"
#include "Sprite.h"
#include "MaskedSprite.h"
#include "NumberUI.h"
#include "NameSpace.h"
#include "GameManager.h"
#include "Scene.h"
#include <algorithm>

namespace {
	void SetupSprite(Sprite* s, const HudBox& b) {
		if (!s) return;
		s->SetSize(HudLayoutUtil::ScreenWidthRatio(b.w), HudLayoutUtil::ScreenHeightRatio(b.h));
		s->SetPosition(HudLayoutUtil::ScreenWidthRatio(b.cx), HudLayoutUtil::ScreenHeightRatio(b.cy));
	}

	void SetupSprite(MaskedSprite* s, const HudBox& b) {
		if (!s) return;
		s->SetSize(HudLayoutUtil::ScreenWidthRatio(b.w), HudLayoutUtil::ScreenHeightRatio(b.h));
		s->SetPosition(HudLayoutUtil::ScreenWidthRatio(b.cx), HudLayoutUtil::ScreenHeightRatio(b.cy));
	}

	void SetupMasked(MaskedSprite* m, const HudBox& b, const std::array<float, 3>& col) {
		if (!m) return;
		SetupSprite(m, b);
		m->SetColor(col[0], col[1], col[2], 1.0f);
		m->SetFeather(0.0f);
		m->SetMaskMode(MaskedSprite::MaskMode::Horizontal);
	}
}

AllyHudPanel::AllyHudPanel(GameObject* owner, const AllyLayout& layout)
	: m_Layout(layout)
{
	// 1. パネル背景
	m_Panel = owner->AddComponent<Sprite>();
	m_Panel->Init(layout.panelPath);

	// 2. HPバー
	m_HpGray = owner->AddComponent<MaskedSprite>();
	m_HpGray->Init(L"Assets/Texture/Dim.png");

	m_HpCol = owner->AddComponent<MaskedSprite>();
	m_HpCol->Init(L"Assets/Texture/Dim.png");

	// 3. 経験値バー
	m_ExpGray = owner->AddComponent<MaskedSprite>();
	m_ExpGray->Init(L"Assets/Texture/Dim.png");

	m_ExpCol = owner->AddComponent<MaskedSprite>();
	m_ExpCol->Init(L"Assets/Texture/Dim.png");

	Scene* scene = GameManager::GetScene();
	// 4. 数値 UI
	if (layout.showHpNum) {
		m_HpNumber = scene->AddGameObject<NumberUI>(eLayer::UI, "AllyHpNumber" + layout.hpFrom);
		m_HpNumber->Init();
	}

	m_LvNumber = scene->AddGameObject<NumberUI>(eLayer::UI, "AllyLvNumber" + layout.hpFrom);
	m_LvNumber->Init();

	ApplyLayout(layout);
}

void AllyHudPanel::ApplyLayout(const AllyLayout& newLayout) 
{
	// メンバ変数を更新
	m_Layout = newLayout;

	// 1. パネル背景
	SetupSprite(m_Panel, m_Layout.panel);

	// 2. HPバー：背景の矩形(barBack)を、手前のバー(bar)にもそのまま適用する
	SetupMasked(m_HpGray, m_Layout.hp.barBack, m_Layout.hp.colBack);

	// barBack の座標とサイズを bar にも強制適用
	HudBox hpMainBox = m_Layout.hp.barBack;
	SetupMasked(m_HpCol, hpMainBox, m_Layout.hp.colBar);

	// 3. 経験値バー：同様に背景に合わせる
	SetupMasked(m_ExpGray, m_Layout.exp.barBack, m_Layout.exp.colBack);

	HudBox expMainBox = m_Layout.exp.barBack;
	SetupMasked(m_ExpCol, expMainBox, m_Layout.exp.colBar);

	if (m_HpNumber) {
		m_HpNumber->SetPosition(
			HudLayoutUtil::ScreenWidthRatio(m_Layout.hp.number.onesDigitX), 
			HudLayoutUtil::ScreenHeightRatio(m_Layout.hp.number.onesDigitY)
		);
		m_HpNumber->SetScale(m_Layout.hp.number.scale);
	}

	if (m_LvNumber) {
		m_LvNumber->SetPosition(
			HudLayoutUtil::ScreenWidthRatio(m_Layout.levelNumber.onesDigitX), 
			HudLayoutUtil::ScreenHeightRatio(m_Layout.levelNumber.onesDigitY)
		);
		m_LvNumber->SetScale(m_Layout.levelNumber.scale);
	}
}

void AllyHudPanel::Update(int hp, int maxHp, int level, float expRatio) {
	if (m_HpCol) {
		float ratio = (maxHp > 0) ? static_cast<float>(hp) / maxHp : 0.0f;
		m_HpCol->SetProgress(std::clamp(ratio, 0.0f, 1.0f));
	}
	if (m_HpNumber) m_HpNumber->SetValue(hp);

	if (m_ExpCol) {
		m_ExpCol->SetProgress(std::clamp(expRatio, 0.0f, 1.0f));
	}
	if (m_LvNumber) m_LvNumber->SetValue(level);
}

void AllyHudPanel::SetVisible(bool visible) {
	float alpha = visible ? 1.0f : 0.0f;
	m_Panel->SetAlpha(alpha);
	m_HpGray->SetAlpha(alpha);
	m_HpCol->SetAlpha(alpha);
	m_ExpGray->SetAlpha(alpha);
	m_ExpCol->SetAlpha(alpha);
	if (m_HpNumber) m_HpNumber->SetAlpha(alpha);
	if (m_LvNumber) m_LvNumber->SetAlpha(alpha);
}