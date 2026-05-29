#include "SkillHudPanel.h"
#include "GameObject.h"
#include "Sprite.h"
#include "MaskedSprite.h"
#include <algorithm>

SkillHudPanel::SkillHudPanel(GameObject* owner, const std::wstring& iconPath, int slotIndex)
{
    const wchar_t* kFrameTex = L"Assets/Texture/Skill/SkillFrame.png";
    const wchar_t* kDimTex = L"Assets/Texture/Dim.png";

    // アイコン本体
    m_Icon = owner->AddComponent<Sprite>();
    m_Icon->Init(iconPath.c_str());

    // クールダウン用の半透明マスク
    m_CdMask = owner->AddComponent<MaskedSprite>();
    m_CdMask->Init(kDimTex);
    m_CdMask->SetColor(0.0f, 0.0f, 0.0f, 0.6f); // 暗めのマスク
    m_CdMask->SetFeather(0.0f);
    m_CdMask->SetMaskMode(MaskedSprite::MaskMode::Vertical);

    // 枠
    m_Frame = owner->AddComponent<Sprite>();
    m_Frame->Init(kFrameTex);
}

void SkillHudPanel::SetLayout(float x, float y, float size) 
{
    auto apply = [&](auto* s) {
        if (!s) return;
        s->SetSize(size, size);
        s->SetPosition(x, y);
        };

    apply(m_Icon);
    apply(m_Frame);
    apply(m_CdMask);
}

void SkillHudPanel::Update(float cdRatio, bool isActive) 
{
    // 1. 表示/非表示の切り替え (透明度で制御)
    float targetAlpha = isActive ? 1.0f : 0.0f;

    if (m_Icon) m_Icon->SetAlpha(targetAlpha);
    if (m_Frame) m_Frame->SetAlpha(targetAlpha);

    // 2. クールダウン情報の更新
    if (isActive && m_CdMask) {
        m_CdMask->SetAlpha(0.6f); // クールダウンマスクの基本透明度
        m_CdMask->SetProgress(std::clamp(cdRatio, 0.0f, 1.0f));
    }
    else if (m_CdMask) {
        m_CdMask->SetAlpha(0.0f);
    }
}

void SkillHudPanel::SetVisible(bool visible) 
{
    float alpha = visible ? 1.0f : 0.0f;
    if (m_Icon) m_Icon->SetAlpha(alpha);
    if (m_Frame) m_Frame->SetAlpha(alpha);
    if (m_CdMask) m_CdMask->SetAlpha(alpha);
}
