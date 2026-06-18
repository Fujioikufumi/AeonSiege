#pragma once
#include "GameObject.h"

/// <summary>
/// 草原やに適用される風の定数バッファ構造体。
/// </summary>
struct alignas(256) CbWind
{
	float Time;			// 時間（秒）
	float WindDirX;		// 風の方向X成分
	float WindDirZ;		// 風の方向Z成分
	float Amplitude;	// 振幅
	float Frequency;	// 周波数
	float Speed;		// 速度
};

/// <summary>
/// フィールド全体の風の状態を管理・更新するオブジェクト。
/// </summary>
class Wind : public GameObject
{
public:
	Wind();
	~Wind() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	//---------------------------------------------------------
	// Setters
	void SetWindXZ(float x, float z) { m_WindDirX = x; m_WindDirZ = z; }
	void SetAmplitude(float amplitude) { m_Amplitude = amplitude; }
	void SetFrequency(float frequency) { m_Frequency = frequency; }
	void SetSpeed(float speed) { m_Speed = speed; }

	//---------------------------------------------------------
	// Getters

	[[nodiscard]] const CbWind& GetCbWind() const { return m_CbWind; }
	[[nodiscard]] float GetWindDirX() const { return m_WindDirX; }
	[[nodiscard]] float GetWindDirZ() const { return m_WindDirZ; }
	[[nodiscard]] float GetAmplitude() const { return m_Amplitude; }
	[[nodiscard]] float GetFrequency() const { return m_Frequency; }
	[[nodiscard]] float GetSpeed() const { return m_Speed; }

private:
	/// 風の定数バッファの更新
	void UpdateCbWind();

private:
	// --- 風の状態パラメータ ---
	float m_Time = 0.0f;
	float m_WindDirX = 1.0f;
	float m_WindDirZ = 0.0f;
	float m_Amplitude = 0.12f;
	float m_Frequency = 0.4f;
	float m_Speed = 1.5f;

	CbWind m_CbWind = {};
};