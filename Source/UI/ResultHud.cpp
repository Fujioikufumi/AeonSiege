#include "UI/ResultHud.h"
#include "Graphics/Sprite/Sprite.h"
#include "UI/NumberUI.h"
#include "Core/GameManager.h"
#include "Core/Scene.h"
#include "Input/Input.h"
#include "Core/NameSpace.h"
#include "UI/TitleScene.h"
#include "Flow/ScreenFade.h"
#include <algorithm>

namespace
{
	constexpr float kNumOffsetX			= 400.0f; // 数値UIのX位置オフセット
	constexpr float kNumScale			= 0.9f;   // 数値UIの拡大率
	constexpr float kReturnBtnWidth		= 150.0f; // 案内の幅
	constexpr float kReturnBtnHeight	= 80.0f;  // 案内の高さ
	constexpr int kFadeInFrames			= 90;	  // フェードインにかけるフレーム数
	constexpr float kDamageNumOffsetY	= 40.0f;  // ダメージ数値のY微調整
	constexpr float kKillsNumOffsetY	= 20.0f;  // キル数値のY微調整

	void SetupNumberUI(NumberUI* num, float x, float y)
	{
		if (num == nullptr) return;

		num->Init();
		num->SetPosition(x, y);
		num->SetTexturePath(L"Assets/Texture/Result/ResultNumber.png");
		num->SetScale(kNumScale);
	}
}

bool ResultHud::Init()
{
	// 背景
	m_pBg = AddComponent<Sprite>();
	m_pBg->Init(L"Assets/Texture/Result/ResultBackGround.png");
	m_pBg->SetPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
	m_pBg->SetSize(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

	// 案内
	m_pImgReturn = AddComponent<Sprite>();
	m_pImgReturn->Init(L"Assets/Texture/Result/ReturnSpace.png");
	m_pImgReturn->SetPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 7.0f / 8.0f);
	m_pImgReturn->SetSize(kReturnBtnWidth, kReturnBtnHeight);

	// 数値UIの追加
	Scene* scene = GameManager::GetScene();
	m_pNumDamage = scene->AddGameObject<NumberUI>(eLayer::MENUUI, "ResNumDamage");
	m_pNumKills = scene->AddGameObject<NumberUI>(eLayer::MENUUI, "ResNumKills");
	m_pNumPhases = scene->AddGameObject<NumberUI>(eLayer::MENUUI, "ResNumPhases");

	// 数値UIの設定
	SetupNumberUI(m_pNumDamage, SCREEN_WIDTH / 2.0f + kNumOffsetX, SCREEN_HEIGHT / 4.0f + kDamageNumOffsetY);
	SetupNumberUI(m_pNumKills,  SCREEN_WIDTH / 2.0f + kNumOffsetX, SCREEN_HEIGHT / 2.0f + kKillsNumOffsetY);
	SetupNumberUI(m_pNumPhases, SCREEN_WIDTH / 2.0f + kNumOffsetX, SCREEN_HEIGHT * 3.0f / 4.0f);

	SetVisible(false);
	return true;
}

void ResultHud::SetVisible(bool visible)
{
	const float alpha = visible ? 1.0f : 0.0f;

	m_pBg->SetAlpha(alpha);
	m_pImgReturn->SetAlpha(alpha);
	if (m_pNumDamage) m_pNumDamage->SetAlpha(alpha);
	if (m_pNumKills)  m_pNumKills->SetAlpha(alpha);
	if (m_pNumPhases) m_pNumPhases->SetAlpha(alpha);
}

void ResultHud::Show()
{
	if (m_IsOpen) return;

	m_IsOpen	= true;
	m_State		= State::Damage;

	// 演出用カウンタをリセット
	m_CurrentKills	= 0.0f;
	m_CurrentPhases = 0.0f;

	const GameStatus& status = GameManager::GetStatus();
	m_TotalDamage	= status.totalDamage;
	m_TotalKills	= status.killCount;
	m_ClearPhases	= status.currentPhase;

	SetVisible(false);
	m_pBg->SetAlpha(1.0f); // 背景だけ先に表示
}

void ResultHud::Update(float deltaTime)
{
	if (!m_IsOpen)
	{
		GameObject::Update(deltaTime);
		return;
	}

	switch (m_State)
	{
	case State::Damage:
		m_pNumDamage->SetAlpha(1.0f);
		m_pNumDamage->SetValue(m_TotalDamage);
		m_State = State::Kills;
		break;

	case State::Kills:
		m_pNumKills->SetAlpha(1.0f);

		// カウントアップ演出
		m_CurrentKills = std::min(
			static_cast<float>(m_TotalKills),
			m_CurrentKills + deltaTime * kKillCountSpeed
		);
		m_pNumKills->SetValue(static_cast<int>(m_CurrentKills));
		if (m_CurrentKills >= static_cast<float>(m_TotalKills))
			m_State = State::Phases;
		break;

	case State::Phases:
		m_pNumPhases->SetAlpha(1.0f);

		// カウントアップ演出
		m_CurrentPhases = std::min(
			static_cast<float>(m_ClearPhases),
			m_CurrentPhases + deltaTime * kPhaseCountSpeed
		);
		m_pNumPhases->SetValue(static_cast<int>(m_CurrentPhases));
		if (m_CurrentPhases >= static_cast<float>(m_ClearPhases))
			m_State = State::Wait;
		break;

	case State::Wait:
		m_pImgReturn->SetAlpha(1.0f);
		if (IsKeyTrigger(VK_SPACE))
		{
			GameManager::ChangeScene(new TitleScene());
			ScreenFade::Instance().FadeIn(kFadeInFrames, nullptr);
		}
		break;
	}

	GameObject::Update(deltaTime);

	// TerrainSceneがリザルト中にシーン更新を止めるため、NumberUIを更新する
	if (m_pNumDamage) m_pNumDamage->Update(deltaTime);
	if (m_pNumKills)  m_pNumKills->Update(deltaTime);
	if (m_pNumPhases) m_pNumPhases->Update(deltaTime);
}