#pragma once
#include "GameObject.h"

//-----------------------------------------------------------------------------
// 		木クラス
//-----------------------------------------------------------------------------
class Tree : public GameObject
{
public:
	Tree();
	~Tree() override;

	bool Init() override;
	void Term() override;
	void Update(float deltaTime) override;
};