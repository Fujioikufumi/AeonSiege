#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
public:
	void Init() override;
	void Term() override;
	void Update(float deltaTime) override;
private:
	class TitleObject* m_TitleObject = nullptr;

	static constexpr float kStartTime = 0.6f; // 決定の入力受付開始までの時間

	float m_CurrentTime = 0.0f;
	int m_CurrentSelectIndex = 0; // 現在選択されている選択肢のインデックス
};