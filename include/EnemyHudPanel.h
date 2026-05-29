#pragma once
#include "CombatHudLayout.h"

class GameObject;
class Sprite;
class MaskedSprite;

class EnemyHudPanel {
public:
    EnemyHudPanel(GameObject* owner, const CombatHudLayout& layout);
    ~EnemyHudPanel() = default;

    void Update(float hpRatio, float deltaTime);

    void SetVisible(bool visible);
    void ApplyLayout();
private:
    const CombatHudLayout& m_Layout; // 色設定などを参照

	Sprite* m_Decor = nullptr;         // HPバーの背景装飾
	MaskedSprite* m_HpGray = nullptr;  // HPバーのグレー部分（最大HP）
	MaskedSprite* m_HpColLag = nullptr;// HPバーの遅れて減る
	MaskedSprite* m_HpCol = nullptr;   // HPバー

    float m_LagRatio = 1.0f; // 遅れて減るバーの現在の割合

	static constexpr float kHpLagDecreaseSpeed = 0.25f; // 遅れて減るバーの減少速度（1秒あたりの割合）
};

