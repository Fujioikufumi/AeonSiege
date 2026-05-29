#pragma once
#include "GameObject.h"

//-----------------------------------------------------------------------------
// 		–ØƒNƒ‰ƒX
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