#pragma once
#include "GameObject.h"
#include "NumberUI.h"
#include <DirectXMath.h>

class Sprite;

/// <summary>
/// 表示するダメージタイプ
/// </summary>
enum class FloatingDamageType
{
	EnemyDamage,
	EnemyCritical,
	AllyDamage,
	AllyCritical,
	ComboDamage,
	ComboCritical,
	Miss
};

class FloatingDamage : public GameObject
{
public:
	FloatingDamage() = default;
	~FloatingDamage() override = default;
	bool Init() override;
	void Update(float deltaTime) override;
	// ダメージ量と初期のスクリーン座標、色を設定する
	void Setup(int damage, float screenX, float screenY, FloatingDamageType type);
private:
	Sprite* m_MissSprite = nullptr; // 回避時の表示UI
	NumberUI* m_NumberUI = nullptr; // ダメージ数値表示用UI
	float m_PosX = 0.0f;
	float m_PosY = 0.0f;
	bool m_IsMiss = false;

	float m_LifeTime = 0.0f;
	const float kMaxLifeTime = 1.0f; // 1秒で消える
	const float kMoveSpeed = 50.0f;  // 上に移動するスピード
	DirectX::XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
};
