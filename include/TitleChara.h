#pragma once
#include "GameObject.h"

/// <summary>
/// タイトル画面で背景の演出キャラクター
/// </summary>
class TitleChara : public GameObject
{
public:
	TitleChara();
	~TitleChara() override = default;
	bool Init() override;
private:
	class AnimationController* m_AnimController = nullptr;
	static constexpr const char* kAnimIdle = "Player_Combat_Idle";
};