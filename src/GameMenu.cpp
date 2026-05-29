#include "GameMenu.h"
#include "Sprite.h"
#include "NameSpace.h"
#include "Input.h"
#include "GameManager.h"
#include "Scene.h"
#include "TitleScene.h"
#include "ScreenFade.h"
#include "GameFlowUtil.h"
#include <algorithm>

bool GameMenu::s_MenuOpen = false;

namespace
{
	// メニューの項目のテクスチャパス
	const wchar_t* kMainItemTexturePaths[4] = {
		L"Assets/Texture/Menu/QuitMenu.png",   // メニューを閉じる
		L"Assets/Texture/Menu/ReturnTitle.png",// タイトルに戻る
		L"Assets/Texture/Menu/ControlsHelp.png",// 操作方法
		L"Assets/Texture/Menu/SkillInfo.png",   // スキル情報
	};

	const wchar_t* kControlsHelpTexturePath = L"Assets/Texture/Menu/HowtoOperate.png";  // 操作方法の画像
	const wchar_t* kSkillInfoTexturePath = L"Assets/Texture/Menu/SkillData.png";		// スキル情報の画像

	// メニューを開く/閉じるトリガー
	bool MenuOpenOrCloseTrigger()
	{
		return IsKeyTrigger(VK_ESCAPE)
			|| IsKeyTrigger('M')
			|| IsControllerTrigger(PAD_BACK);
	}

	// メニューで決定するトリガー
	bool MenuConfirmTrigger()
	{
		return IsKeyTrigger(VK_SPACE) || IsKeyTrigger(VK_RETURN) || IsControllerTrigger(PAD_A);
	}

	// メニューで上下に移動するトリガー
	bool MenuUpTrigger()
	{
		return IsKeyTrigger(VK_UP) || IsKeyTrigger('W') || IsControllerTrigger(PAD_UP);
	}

	// メニューで下に移動するトリガー
	bool MenuDownTrigger()
	{
		return IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S') || IsControllerTrigger(PAD_DOWN);
	}
}

bool GameMenu::IsMenuOpen()
{
	return s_MenuOpen;
}

bool GameMenu::Init()
{
	// メニュー全体の背景
	m_Dim = AddComponent<Sprite>();
	if (!m_Dim->Init(L"Assets/Texture/Shop/ShopBackGround.png"))
	{
		ELOG("GameMenu: failed to init dim sprite");
		return false;
	}
	m_Dim->SetPosition(static_cast<float>(SCREEN_WIDTH) * 0.5f, static_cast<float>(SCREEN_HEIGHT) * 0.5f);
	m_Dim->SetSize(static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));
	m_Dim->SetColor(0.0f, 0.0f, 0.0f, 0.55f);
	SetSpriteVisible(m_Dim, false);

	// メニューの項目スプライト
	for (int i = 0; i < static_cast<int>(MainItem::Count); ++i)
	{
		Sprite* spr = AddComponent<Sprite>();
		if (!spr->Init(kMainItemTexturePaths[i]))
		{
			ELOG("GameMenu: failed to init main item sprite %d", i);
			return false;
		}
		const float cx = kMainItemBaseX;
		const float cy = kMainItemBaseY + kMainItemStepY * static_cast<float>(i);
		spr->SetPosition(cx, cy);
		spr->SetScale(kNormalScale, kNormalScale);
		m_MainItemSprites[static_cast<size_t>(i)] = spr;
		m_MainItemRestPos[static_cast<size_t>(i)] = DirectX::XMFLOAT2(cx, cy);
		SetSpriteVisible(spr, false);
	}

	// 操作方法
	m_ControlsHelpSprite = AddComponent<Sprite>();
	if (!m_ControlsHelpSprite->Init(kControlsHelpTexturePath))
	{
		ELOG("GameMenu: failed to init controls help sprite");
		return false;
	}
	m_ControlsHelpSprite->SetPosition(static_cast<float>(SCREEN_WIDTH) * 0.5f, static_cast<float>(SCREEN_HEIGHT) * 0.5f);
	m_ControlsHelpSprite->SetSize(kPanelSize);
	m_ControlsHelpRestPos = m_ControlsHelpSprite->GetPosition();
	SetSpriteVisible(m_ControlsHelpSprite, false);

	// スキル情報
	m_SkillInfoSprite = AddComponent<Sprite>();
	if (!m_SkillInfoSprite->Init(kSkillInfoTexturePath))
	{
		ELOG("GameMenu: failed to init skill info sprite");
		return false;
	}
	m_SkillInfoSprite->SetPosition(m_ControlsHelpRestPos.x, m_ControlsHelpRestPos.y);
	m_SkillInfoSprite->SetSize(kPanelSize);
	m_SkillInfoRestPos = m_SkillInfoSprite->GetPosition();
	SetSpriteVisible(m_SkillInfoSprite, false);

	m_IsMenuOpen = false;
	s_MenuOpen = false;
	m_Panel = Panel::Main;
	m_MainSelectIndex = 0;

	return true;
}

void GameMenu::Update(float deltaTime)
{
	// ショップ中はゲームメニューを扱わない（必要なら Shop 側でメニューを閉じる処理を追加）
	if (GameFlowUtil::IsShopOpen())
	{
		if (m_IsMenuOpen)
		{
			CloseMenu();
		}
		GameObject::Update(deltaTime);
		return;
	}

	// --- 閉じているとき：トグルで開く ---
	if (!m_IsMenuOpen)
	{
		if (MenuOpenOrCloseTrigger())
		{
			OpenMenu();
		}
		GameObject::Update(deltaTime);
		return;
	}

	if (m_IsMenuOpen && m_MenuIntroPlaying)
	{
		UpdateMenuIntro(deltaTime);
		GameObject::Update(deltaTime);
		return;
	}

	// --- 開いているとき ---
	if (m_Panel == Panel::ControlsHelp)
	{
		if (MenuOpenOrCloseTrigger() || MenuConfirmTrigger())
		{
			ReturnToMainPanel();
		}
		GameObject::Update(deltaTime);
		return;
	}

	if (m_Panel == Panel::SkillInfo)
	{
		if (MenuOpenOrCloseTrigger() || MenuConfirmTrigger())
		{
			ReturnToMainPanel();
		}
		GameObject::Update(deltaTime);
		return;
	}

	// Panel::Main
	if (MenuOpenOrCloseTrigger())
	{
		CloseMenu();
		GameObject::Update(deltaTime);
		return;
	}

	if (MenuUpTrigger())
	{
		m_MainSelectIndex--;
		if (m_MainSelectIndex < 0)
		{
			m_MainSelectIndex = static_cast<int>(MainItem::Count) - 1;
		}
		RefreshSelectVisual();
	}
	else if (MenuDownTrigger())
	{
		m_MainSelectIndex++;
		if (m_MainSelectIndex >= static_cast<int>(MainItem::Count))
		{
			m_MainSelectIndex = 0;
		}
		RefreshSelectVisual();
	}
	else if (MenuConfirmTrigger())
	{
		switch (static_cast<MainItem>(m_MainSelectIndex))
		{
		case MainItem::CloseMenu:
			CloseMenu();
			break;
		case MainItem::ReturnToTitle:
			CloseMenu();
			GameManager::ChangeScene(new TitleScene());
			ScreenFade::Instance().FadeIn(90, nullptr);
			break;
		case MainItem::Controls:
			EnterControlsHelp();
			break;
		case MainItem::SkillInfo:
			EnterSkillInfo();
			break;
		default:
			break;
		}
	}

	GameObject::Update(deltaTime);
}

void GameMenu::OpenMenu()
{
	m_IsMenuOpen = true;
	s_MenuOpen = true;
	m_Panel = Panel::Main;
	m_MainSelectIndex = 0;
	ApplyPanelLayout();
	RefreshSelectVisual();
	BeginMenuIntro();
}

void GameMenu::CloseMenu()
{
	m_IsMenuOpen = false;
	s_MenuOpen = false;
	m_Panel = Panel::Main;
	m_MenuIntroPlaying = false;
	RestPositions();
	SetSpriteVisible(m_Dim, false);
	for (Sprite* spr : m_MainItemSprites)
	{
		SetSpriteVisible(spr, false);
	}
	SetSpriteVisible(m_ControlsHelpSprite, false);
	SetSpriteVisible(m_SkillInfoSprite, false);
}

void GameMenu::EnterControlsHelp()
{
	m_Panel = Panel::ControlsHelp;
	ApplyPanelLayout();
	BeginMenuIntro();
}

void GameMenu::EnterSkillInfo()
{
	m_Panel = Panel::SkillInfo;
	ApplyPanelLayout();
	BeginMenuIntro();
}

void GameMenu::ReturnToMainPanel()
{
	m_Panel = Panel::Main;
	ApplyPanelLayout();
	RefreshSelectVisual();
	BeginMenuIntro();
}

void GameMenu::ApplyPanelLayout()
{
	if (!m_IsMenuOpen)
	{
		return;
	}

	switch (m_Panel)
	{
	case Panel::Main:
		SetSpriteVisible(m_Dim, true);
		for (Sprite* spr : m_MainItemSprites)
		{
			SetSpriteVisible(spr, true);
		}
		SetSpriteVisible(m_ControlsHelpSprite, false);
		SetSpriteVisible(m_SkillInfoSprite, false);
		break;
	case Panel::ControlsHelp:
		SetSpriteVisible(m_Dim, true);
		for (Sprite* spr : m_MainItemSprites)
		{
			SetSpriteVisible(spr, false);
		}
		SetSpriteVisible(m_ControlsHelpSprite, true);
		SetSpriteVisible(m_SkillInfoSprite, false);
		break;
	case Panel::SkillInfo:
		SetSpriteVisible(m_Dim, true);
		for (Sprite* spr : m_MainItemSprites)
		{
			SetSpriteVisible(spr, false);
		}
		SetSpriteVisible(m_ControlsHelpSprite, false);
		SetSpriteVisible(m_SkillInfoSprite, true);
		break;
	default:
		break;
	}
}

void GameMenu::RefreshSelectVisual()
{
	for (int i = 0; i < static_cast<int>(MainItem::Count); ++i)
	{
		Sprite* spr = m_MainItemSprites[static_cast<size_t>(i)];
		if (spr == nullptr)
		{
			continue;
		}
		if (i == m_MainSelectIndex)
		{
			spr->SetScale(kSelectedScale, kSelectedScale);
		}
		else
		{
			spr->SetScale(kNormalScale, kNormalScale);
		}
	}
}

void GameMenu::SetSpriteAlpha(Sprite* sprite, float alpha)
{
	if (sprite == nullptr)
		return;
	if (alpha >= 1.0f)
		sprite->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	else if (alpha <= 0.0f)
		sprite->SetColor(1.0f, 1.0f, 1.0f, 0.0f);
	else
		sprite->SetColor(1.0f, 1.0f, 1.0f, alpha);
}

void GameMenu::SetSpriteVisible(Sprite* sprite, bool visible)
{
	if (sprite == nullptr)
	{
		return;
	}
	SetSpriteAlpha(sprite, visible ? 1.0f : 0.0f);
}

void GameMenu::BeginMenuIntro()
{
	m_MenuIntroElapsed = 0.0f;
	m_MenuIntroPlaying = true;
	UpdateMenuIntro(0.0f);
}

void GameMenu::UpdateMenuIntro(float deltaTime)
{
	m_MenuIntroElapsed += deltaTime;
	const float t = std::min(1.0f, m_MenuIntroElapsed / kMenuIntroDurationSec);

	auto lerpX = [](float restX, float fromX, float tt) {
		return fromX + (restX - fromX) * tt;
		};

	switch (m_Panel)
	{
	case Panel::Main:
		for (int i = 0; i < static_cast<int>(MainItem::Count); ++i)
		{
			Sprite* spr = m_MainItemSprites[static_cast<size_t>(i)];
			if (spr == nullptr) continue;
			const XMFLOAT2& rest = m_MainItemRestPos[static_cast<size_t>(i)];
			const float fromX = rest.x - kMenuSlideOffsetPx;
			spr->SetPosition(lerpX(rest.x, fromX, t), rest.y);
		}
		break;
	case Panel::ControlsHelp:
		if (m_ControlsHelpSprite != nullptr)
		{
			const float fromX = m_ControlsHelpRestPos.x + kMenuSlideOffsetPx;
			m_ControlsHelpSprite->SetPosition(
				lerpX(m_ControlsHelpRestPos.x, fromX, t),
				m_ControlsHelpRestPos.y);
		}
		break;
	case Panel::SkillInfo:
		if (m_SkillInfoSprite != nullptr)
		{
			const float fromX = m_SkillInfoRestPos.x + kMenuSlideOffsetPx;
			m_SkillInfoSprite->SetPosition(
				lerpX(m_SkillInfoRestPos.x, fromX, t),
				m_SkillInfoRestPos.y);
		}
		break;
	default:
		break;
	}

	if (t >= 1.0f)
	{
		m_MenuIntroPlaying = false;
		RestPositions();
	}
}

void GameMenu::RestPositions()
{
	for (int i = 0; i < static_cast<int>(MainItem::Count); ++i)
	{
		Sprite* spr = m_MainItemSprites[static_cast<size_t>(i)];
		if (spr == nullptr) continue;
		const XMFLOAT2& rest = m_MainItemRestPos[static_cast<size_t>(i)];
		spr->SetPosition(rest.x, rest.y);
	}
	if (m_ControlsHelpSprite != nullptr)
		m_ControlsHelpSprite->SetPosition(m_ControlsHelpRestPos.x, m_ControlsHelpRestPos.y);
	if (m_SkillInfoSprite != nullptr)
		m_SkillInfoSprite->SetPosition(m_SkillInfoRestPos.x, m_SkillInfoRestPos.y);
}