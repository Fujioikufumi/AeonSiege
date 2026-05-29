#pragma once
#include "GameObject.h"
#include <functional>
#include <memory>

struct RenderContext;

// 画面全体のフェード
class ScreenFade
{
public:
	static ScreenFade& Instance();

	bool Init();
	void Term();
	void Update(float deltaTime);
	void Draw(const RenderContext& context);

	// frameCount : フェードにかかるフレーム数（0 以下なら 1 扱い）
	void FadeIn(int frameCount, std::function<void()> onComplete = nullptr);
	void FadeOut(int frameCount, std::function<void()> onComplete = nullptr);

	bool IsActive() const { return m_Mode != Mode::None; }

private:
	ScreenFade() = default;

	enum class Mode
	{
		None,
		FadeIn,
		FadeOut,
	};

	void ResetState();
	void StartFade(Mode mode, int frameCount, std::function<void()> onComplete, float startAlpha);

	std::unique_ptr<GameObject> m_pRoot;
	class Sprite* m_pSprite = nullptr;

	Mode m_Mode = Mode::None;
	int m_CurrentFrame = 0;
	int m_EndFrame = 1;
	std::function<void()> m_OnComplete;
};