#pragma once

class GameObject;
class Sprite;
class NumberUI;

// 画面右上の「Phase X」表示
class PhaseHudPanel
{
public:
	explicit PhaseHudPanel(GameObject* owner);
	~PhaseHudPanel() = default;

	void Update(int phaseNo);
	void ApplyLayout();

private:
	Sprite* m_Label = nullptr;		// Phaseテクスチャ
	NumberUI* m_Number = nullptr;  // フェーズ数表示

	static constexpr float kLabelCenterX = 0.83f;
	static constexpr float kLabelCenterY = 0.07f;
	static constexpr float kLabelWidth = 0.08f;
	static constexpr float kLabelHeight = 0.05f;

	static constexpr float kNumberOnesX = 0.985f; // 一の位基準（右寄せ）
	static constexpr float kNumberY = 0.07f;
	static constexpr float kNumberScale = 0.4f;

	static constexpr wchar_t kPhaseLabelTex[] = L"Assets/Texture/CombatHud/Phase.png";
	static constexpr wchar_t kNumberTex[] = L"Assets/Texture/Number/PhaseNumber.png";
};