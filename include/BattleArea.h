#pragma once
#include "GameObject.h"
#include <DirectXMath.h>

/// <summary>
/// 戦闘エリア
/// </summary>
class BattleArea : public GameObject
{
public:
	BattleArea();
	~BattleArea() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	// 中心と半径を設定する。
	void Setup(const DirectX::XMFLOAT3& center, float radius);
	const DirectX::XMFLOAT3& GetCenter() const noexcept { return m_Center; }
	float GetRadius() const noexcept { return m_Radius; }
private:
	void ClampLayerObjects(eLayer layer) const;
	void ClampObjectToArea(GameObject* object) const;

private:
	DirectX::XMFLOAT3 m_Center = { 0.0f, 0.0f, 0.0f };
	float m_Radius = 500.0f;
};
