#pragma once
#include "Component.h"
#include "GameObject.h"
#include <DirectXMath.h>
using namespace DirectX;

//-----------------------------------------------------------------------------
//		移動コンポーネントクラス
//-----------------------------------------------------------------------------
class MovementComponent : public Component
{
public:
	MovementComponent(GameObject* obj);
	~MovementComponent() override;

	/// <summary>
	/// 初期化処理
	/// </summary>
	bool Init();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime) override;
private:

};