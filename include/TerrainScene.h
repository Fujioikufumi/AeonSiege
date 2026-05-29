#pragma once
#include "Scene.h"
#include "Terrain.h"
class TerrainScene : public Scene
{
public:
	void Init() override;
	void Term() override;
	void Update(float deltaTime) override;
	//void Draw(const RenderContext& context) override;
private:
	Terrain* m_Terrain = nullptr;				// 地形オブジェクト
	class ResultHud* m_ResultHud = nullptr;		// リザルトHUD
};