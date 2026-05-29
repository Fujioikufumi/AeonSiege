#pragma once
#include <string>

class GameObject;
class Sprite;
class MaskedSprite;

/// <summary>
/// スキルのHUDを管理するクラス
/// </summary>
class SkillHudPanel 
{
public:
    SkillHudPanel(GameObject* owner, const std::wstring& iconPath, int slotIndex);
    ~SkillHudPanel() = default;

    void Update(float cdRatio, bool isActive);

    void SetLayout(float x, float y, float size);

    void SetVisible(bool visible);

private:
	Sprite* m_Icon = nullptr;  // スキルアイコン
	Sprite* m_Frame = nullptr; // アイコンの枠
    MaskedSprite* m_CdMask = nullptr; // クールダウンの表示
};

