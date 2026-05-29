#include "ScreenFade.h"
#include <algorithm>
#include "Sprite.h"
#include "RenderContext.h"
#include "main.h"

ScreenFade& ScreenFade::Instance()
{
	static ScreenFade s;
	return s;
}

void ScreenFade::ResetState()
{
	m_Mode = Mode::None;
	m_CurrentFrame = 0;
	m_EndFrame = 1;
	m_OnComplete = nullptr;
}

void ScreenFade::StartFade(Mode mode, int frameCount, std::function<void()> onComplete, float startAlpha)
{
	if (m_pSprite == nullptr)
	{
		return;
	}

	m_Mode = mode;
	m_CurrentFrame = 0;
	m_EndFrame = max(1, frameCount);
	m_OnComplete = std::move(onComplete);
	m_pSprite->SetColor(0.0f, 0.0f, 0.0f, startAlpha);
}

bool ScreenFade::Init()
{
	Term();

	m_pRoot = std::make_unique<GameObject>();
	m_pRoot->SetLayer(eLayer::UI);

	m_pSprite = m_pRoot->AddComponent<Sprite>();
	if (!m_pSprite->Init(L"Assets/Texture/Dim.png"))
	{
		m_pSprite = nullptr;
		m_pRoot.reset();
		return false;
	}

	m_pSprite->SetPosition(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f);
	m_pSprite->SetSize(static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));
	m_pSprite->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	m_pRoot->Update(0.0f);
	ResetState();
	return true;
}

void ScreenFade::Term()
{
	if (m_pRoot)
	{
		m_pRoot->Term();
		m_pRoot.reset();
	}

	m_pSprite = nullptr;
	ResetState();
}

void ScreenFade::Update(float deltaTime)
{
	if (m_Mode == Mode::None || m_pSprite == nullptr)
	{
		return;
	}

	const int endSafe = max(1, m_EndFrame);

	if (m_CurrentFrame >= endSafe)
	{
		m_Mode = Mode::None;
		if (m_OnComplete)
		{
			auto cb = std::move(m_OnComplete);
			m_OnComplete = nullptr;
			cb();
		}
		return;
	}

	++m_CurrentFrame;

	float alpha = 0.0f;
	switch (m_Mode)
	{
	case Mode::FadeIn:
		alpha = 1.0f - static_cast<float>(m_CurrentFrame) / static_cast<float>(endSafe);
		break;
	case Mode::FadeOut:
		alpha = static_cast<float>(m_CurrentFrame) / static_cast<float>(endSafe);
		break;
	default:
		break;
	}

	alpha = std::clamp(alpha, 0.0f, 1.0f);
	m_pSprite->SetColor(0.0f, 0.0f, 0.0f, alpha);
	m_pRoot->Update(0.0f);
}

void ScreenFade::Draw(const RenderContext& context)
{
	if (m_pRoot == nullptr)
	{
		return;
	}
	m_pRoot->Draw(context);
}

void ScreenFade::FadeIn(int frameCount, std::function<void()> onComplete)
{
	StartFade(Mode::FadeIn, frameCount, std::move(onComplete), 1.0f);
}

void ScreenFade::FadeOut(int frameCount, std::function<void()> onComplete)
{
	StartFade(Mode::FadeOut, frameCount, std::move(onComplete), 0.0f);
}