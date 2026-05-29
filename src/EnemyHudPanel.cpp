#include "EnemyHudPanel.h"
#include "GameObject.h"
#include "Sprite.h"
#include "MaskedSprite.h"
#include "NameSpace.h"
#include <algorithm>

EnemyHudPanel::EnemyHudPanel(GameObject* owner, const CombatHudLayout& layout)
	: m_Layout(layout)
{
	const wchar_t* kEnemyBackTex = L"Assets/Texture/CombatHud/TargetHpBarBack.png";
	const wchar_t* kDimTex = L"Assets/Texture/Dim.png";

	m_Decor = owner->AddComponent<Sprite>();
	m_Decor->Init(kEnemyBackTex);

	// ステータスバーの共通設定をラムダでまとめる
	auto setupBar = [&](MaskedSprite*& m, const std::array<float, 3>& col) {
		m = owner->AddComponent<MaskedSprite>();
		m->Init(kDimTex);
		m->SetColor(col[0], col[1], col[2], 1.0f);
		m->SetFeather(0.0f);
		m->SetMaskMode(MaskedSprite::MaskMode::Horizontal);
		};

	setupBar(m_HpGray, layout.colGray);
	setupBar(m_HpColLag, layout.colHpLag);
	setupBar(m_HpCol, layout.colHp);

	// 初期レイアウト適用
	m_Decor->SetSize(HudLayoutUtil::ScreenWidthRatio(layout.enemyDecor.w), HudLayoutUtil::ScreenHeightRatio(layout.enemyDecor.h));
	m_Decor->SetPosition(HudLayoutUtil::ScreenWidthRatio(layout.enemyDecor.cx), HudLayoutUtil::ScreenHeightRatio(layout.enemyDecor.cy));

	// 全バーで同じ Box を共有するためのラムダ
	auto setBarLayout = [&](MaskedSprite* m) {
		m->SetSize(
			HudLayoutUtil::ScreenWidthRatio(layout.enemyBar.w), 
			HudLayoutUtil::ScreenHeightRatio(layout.enemyBar.h)
		);
		m->SetPosition(
			HudLayoutUtil::ScreenWidthRatio(layout.enemyBar.cx), 
			HudLayoutUtil::ScreenHeightRatio(layout.enemyBar.cy)
		);
	};
	setBarLayout(m_HpGray);
	setBarLayout(m_HpColLag);
	setBarLayout(m_HpCol);
}

void EnemyHudPanel::Update(float hpRatio, float deltaTime) 
{
	hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);

	// メインのHPバーは即座に反映
	if (m_HpCol) m_HpCol->SetProgress(hpRatio);

	// ラグバーの計算 (徐々に hpRatio に近づける)
	if (m_LagRatio > hpRatio) {
		m_LagRatio -= kHpLagDecreaseSpeed * deltaTime; // 減少速度
		if (m_LagRatio < hpRatio) m_LagRatio = hpRatio;
	}
	else {
		m_LagRatio = hpRatio; // 回復時は即座に追いつく
	}

	if (m_HpColLag) m_HpColLag->SetProgress(m_LagRatio);
	if (m_HpGray)   m_HpGray->SetProgress(1.0f);
}

void EnemyHudPanel::SetVisible(bool visible) 
{
	float alpha = visible ? 1.0f : 0.0f;

	m_Decor->SetAlpha(alpha);
	if (m_HpGray) m_HpGray->SetAlpha(alpha);
	if (m_HpColLag) m_HpColLag->SetAlpha(alpha);
	if (m_HpCol) m_HpCol->SetAlpha(alpha);
}

void EnemyHudPanel::ApplyLayout() 
{
	// 全体の装飾設定
	m_Decor->SetSize(
		HudLayoutUtil::ScreenWidthRatio(m_Layout.enemyDecor.w), 
		HudLayoutUtil::ScreenHeightRatio(m_Layout.enemyDecor.h)
	);
	m_Decor->SetPosition(
		HudLayoutUtil::ScreenWidthRatio(m_Layout.enemyDecor.cx), 
		HudLayoutUtil::ScreenHeightRatio(m_Layout.enemyDecor.cy)
	);

	// 全バー（背景、ラグ、メイン）で同じ Box を共有する
	const HudBox& box = m_Layout.enemyBar;

	auto apply = [&](MaskedSprite* m) {
		if (!m) return;
		m->SetSize(
			HudLayoutUtil::ScreenWidthRatio(box.w), 
			HudLayoutUtil::ScreenHeightRatio(box.h)
		);
		m->SetPosition(
			HudLayoutUtil::ScreenWidthRatio(box.cx), 
			HudLayoutUtil::ScreenHeightRatio(box.cy)
		);
	};

	apply(m_HpGray);
	apply(m_HpColLag);
	apply(m_HpCol);
}