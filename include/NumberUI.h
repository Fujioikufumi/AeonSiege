#pragma once
#include "GameObject.h"
#include <vector>
#include <string>

/// <summary>
/// スプライトを用いて画面上に数値を表示するUIオブジェクト。
/// 一桁ごとにスプライトを作成する
/// </summary>
class NumberUI : public GameObject
{
public:
	NumberUI() = default;
	~NumberUI() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	//表示設定
	void SetValue(int value); // 既に設定している値と異なる桁の場合新たにスプライトを追加する
	void SetPosition(float x, float y);
	void SetScale(float scale);
	void SetColor(float r, float g, float b, float a = 1.0f);
	void SetColor(const DirectX::XMFLOAT4& color);
	void SetAlpha(float a) { m_Color.w = a; }
	void SetTexturePath(const std::wstring& path);

	// --- ゲッター ---
	[[nodiscard]] DirectX::XMFLOAT4 GetColor() const { return m_Color; }

private:
	std::wstring m_TexturePath = L"Assets/Texture/Number/Default.png";
	DirectX::XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_PosX = 0.0f;
	float m_PosY = 0.0f;
	float m_Scale = 1.0f;

	int m_CurrentValue = -1;
	std::vector<class Sprite*> m_DigitSprites; // 各桁のスプライト

	static constexpr float kDigitWidth = 93.0f;
	static constexpr float kDigitHeight = 170.0f;
	static constexpr float kUVStep = 0.1f;    // 0~9の10分割
};