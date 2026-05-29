#pragma once
#include "GameObject.h"

//-----------------------------------------------------------------------------
// ロックオンUI : ロックオンしている敵に表示されるUI
//-----------------------------------------------------------------------------
class LockOnUI : public GameObject
{
public:
	LockOnUI() = default;
	~LockOnUI() override = default;

	bool Init() override;
	void Update(float deltaTime) override;
};