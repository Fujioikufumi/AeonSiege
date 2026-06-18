#pragma once
#include "GameObject.h"

class Sprite;
class NumberUI;

/// <summary>
/// ゲーム終了時のリザルト演出 HUD
/// </summary>
class ResultHud : public GameObject
{
public:
	ResultHud() = default;
	~ResultHud() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	void Show();
	bool IsOpen() const { return m_IsOpen; }

private:
	enum class State
	{
		Damage, // 総ダメージ表示
		Kills,  // キル数カウントアップ
		Phases, // フェーズ数カウントアップ
		Wait,   // 案内表示・入力待ち
	};

	void SetVisible(bool visible);

	// 背景・案内
	Sprite* m_pBg = nullptr;
	Sprite* m_pImgReturn = nullptr;

	// 数値UI※Scene 上の GameObject。TerrainScene が全体更新を止めるため Update は自前で呼ぶ
	NumberUI* m_pNumDamage = nullptr;
	NumberUI* m_pNumKills = nullptr;
	NumberUI* m_pNumPhases = nullptr;

	bool m_IsOpen = false;
	State m_State = State::Damage;

	float m_CurrentKills = 0.0f;  // カウントアップ演出用
	float m_CurrentPhases = 0.0f;

	int m_TotalDamage = 0;
	int m_TotalKills = 0;
	int m_ClearPhases = 0;

	static constexpr float kKillCountSpeed = 70.0f;   // キル数の増加速度
	static constexpr float kPhaseCountSpeed = 40.0f;  // フェーズ数の増加速度
	static constexpr float kNumberScale = 0.9f;
};