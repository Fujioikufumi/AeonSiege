#pragma once
#include "Component.h"
#include <DirectXMath.h>
#include "GameObject.h"
/// <summary>
/// ロックオンの目標を示すコンポーネント (戦闘対象の登録)
/// </summary>
class TargetComponent : public Component
{
public:
	TargetComponent(GameObject* pObj) : Component(pObj) {}
	virtual ~TargetComponent() override = default;

	bool Init() override { return true; }

	// ロックオン時の目標座標(カメラの注視点)を返す
	DirectX::XMFLOAT3 GetLockPosition() const
	{
		DirectX::XMFLOAT3 pos = m_GameObject->GetPosition();
		pos.y += m_HeightOffset;
		return pos;
	}

	float GetHeightOffset() const { return m_HeightOffset; }
	void SetHeightOffset(float offset) { m_HeightOffset = offset; }

private:
	float m_HeightOffset = 10.0f;
};