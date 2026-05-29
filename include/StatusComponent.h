#pragma once
#include "Component.h"
#include "StatusData.h"

/// <summary>
/// 戦闘キャラクターのステータスを管理するコンポーネント。
/// なお、生存確認などはHealthComponentが担当する。
/// </summary>
class StatusComponent : public Component
{
public:
	StatusComponent(GameObject* pObj);
	~StatusComponent() override = default;

	bool Init() override;
	void Setup(const StatusData& data);

	const StatusData& GetBaseStatus() const { return m_Status; }

	//---------------------------------------------------------
	// ステータス上昇メソッド

	void AddAttackRate(float value);
	void AddMaxHpRate(float value);
	void AddMoveSpeedRate(float value);
	void AddDamageReductionRate(float value);
	void AddSkillPowerRate(float value);
	void AddSkillCooldownReductionRate(float value);
	void AddSkillDurationRate(float value);
	void AddCriticalRate(float value);
	void AddCriticalDamageRate(float value);
	void AddEvasionRate(float value);
	void AddSkillChainDamageRate(float value);
	void AddSkillRangeRate(float value);

	//---------------------------------------------------------
	//  getters

	// ステータス倍率のゲッター
	[[nodiscard]] float GetAttackRate() const { return m_Status.attackRate; }
	[[nodiscard]] float GetMaxHpRate() const { return m_Status.maxHpRate; }
	[[nodiscard]] float GetMoveSpeedRate() const { return m_Status.moveSpeedRate; }
	[[nodiscard]] float GetDamageTakenRate() const { return m_Status.damageTakenRate; }
	[[nodiscard]] float GetCriticalRate() const { return m_Status.criticalRate; }
	[[nodiscard]] float GetSkillCooldownRate() const { return m_Status.skillCooldownRate; }
	[[nodiscard]] float GetCriticalDamageRate() const { return m_Status.criticalDamageRate; }
	[[nodiscard]] float GetEvasionRate() const { return m_Status.evasionRate; }
	[[nodiscard]] float GetSkillChainDamageRate() const { return m_Status.skillChainDamageRate; }
	[[nodiscard]] float GetSkillRangeRate() const { return m_Status.skillRangeRate; }

	// ステータスの実数値のゲッター
	[[nodiscard]] float GetMoveSpeed() const { return m_Status.moveSpeed * m_Status.moveSpeedRate; }
	[[nodiscard]] int GetMaxHp() const;
	[[nodiscard]] int GetAttackPower() const;
	[[nodiscard]] float GetAutoAttackInterval() const;

	// ダメージ計算
	[[nodiscard]] int CalculateTakenDamage(int baseDamage) const;
	[[nodiscard]] int CalculateSkillDamage(float skillPowerRate) const;
	[[nodiscard]] float CalculateSkillDuration(float baseDuration) const;
private:
	// クランプ処理
	static void ClampMin(float& value, float minValue);
	static void ClampRate(float& value, float minValue, float delta, bool add = true);

	StatusData m_Status;
};