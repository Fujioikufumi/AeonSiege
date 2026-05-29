#pragma once
#include "GameObject.h"
#include <DirectXMath.h>

class Terrain;

class BattleAreaObj : public GameObject
{
public:
	BattleAreaObj();
	~BattleAreaObj() override = default;

	bool Init() override;
	void Term() override;

	void Setup(const DirectX::XMFLOAT3& center, float radius);
private:
	void PlaceBoundaryTrees();
	float RandomRange(float minValue, float maxValue) const;

private:
	DirectX::XMFLOAT3 m_Center = { 0.0f, 0.0f, 0.0f };
	float m_Radius = 600.0f;
	bool m_HasPlaced = false;

	static constexpr int kTreeCount = 36;
	static constexpr float kInnerRadiusOffset = -35.0f;
	static constexpr float kOuterRadiusOffset = 25.0f;
	static constexpr float kAngleRandomRange = 0.18f;
	static constexpr float kMinTreeScale = 0.08f;
	static constexpr float kMaxTreeScale = 0.13f;
};