#pragma once
#include "GameObject.h"
#include "Sprite.h"

/// <summary>
/// タイトルシーンのUI表示（ロゴ、選択肢）を管理する
/// </summary>
class TitleObject : public GameObject
{
public:
    TitleObject();
    ~TitleObject() override;

    bool Init() override;
    void Term() override;

    void Update(float deltaTime) override;
    
    void SetSelectIndex(int index) { m_CurrentSelectIndex = index; };
private:
    Sprite* m_pSprite = nullptr;  // タイトルを表示するためのコンポ―ネント
    Sprite* m_SelectMode[2] = { nullptr, nullptr }; // スタートかゲームを終了するかの選択肢

	int m_CurrentSelectIndex = 0; // 現在選択されている選択肢のインデックス
	static constexpr int kSelectModeCount = 2; // 選択肢の数
};