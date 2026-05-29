#include "StatusComponent.h"
#include <algorithm>

StatusComponent::StatusComponent(GameObject* pObj)
	: Component(pObj)
{
	m_ComponentName = "StatusComponent";
}

bool StatusComponent::Init()
{
	return true;
}

void StatusComponent::Setup(const StatusData& data)
{
	m_Status = data;
}

void StatusComponent::ClampMin(float& value, float minValue)
{
	value = max(minValue, value);
}

void StatusComponent::ClampRate(float& value, float minValue, float delta, bool add)
{
	// ’l‚ð‰ÁŽZ‚Ü‚½‚ÍŒ¸ŽZ
	value = add ? (value + delta) : (value - delta);
	ClampMin(value, minValue);
}

void StatusComponent::AddAttackRate(float value)
{
	ClampRate(m_Status.attackRate, 0.0f, value);
}

void StatusComponent::AddMaxHpRate(float value)
{
	ClampRate(m_Status.maxHpRate, 0.1f, value);
}

void StatusComponent::AddMoveSpeedRate(float value)
{
	ClampRate(m_Status.moveSpeedRate, 0.1f, value);
}

void StatusComponent::AddDamageReductionRate(float value)
{
	ClampRate(m_Status.damageTakenRate, 0.1f, value, false);
}

int StatusComponent::GetMaxHp() const
{
	return static_cast<int>(m_Status.maxHp * m_Status.maxHpRate);
}

int StatusComponent::GetAttackPower() const
{
	return static_cast<int>(m_Status.attackPower * m_Status.attackRate);
}

float StatusComponent::GetAutoAttackInterval() const
{
	return m_Status.autoAttackInterval;
}

int StatusComponent::CalculateTakenDamage(int baseDamage) const
{
	const int damage = static_cast<int>(baseDamage * m_Status.damageTakenRate);
	return max(1, damage);
}

void StatusComponent::AddSkillPowerRate(float value)
{
	ClampRate(m_Status.skillPowerRate, 0.0f, value);
}

void StatusComponent::AddSkillCooldownReductionRate(float value)
{
	ClampRate(m_Status.skillCooldownRate, 0.1f, value, false);
}

int StatusComponent::CalculateSkillDamage(float skillPowerRate) const
{
	const float damage =
		static_cast<float>(GetAttackPower()) *
		skillPowerRate *
		m_Status.skillPowerRate;
	return static_cast<int>(damage);
}

void StatusComponent::AddCriticalRate(float value)
{
	m_Status.criticalRate = std::clamp(m_Status.criticalRate + value, 0.0f, 1.0f);
}

void StatusComponent::AddCriticalDamageRate(float value)
{
	ClampMin(m_Status.criticalDamageRate, 1.0f);
	m_Status.criticalDamageRate += value;
}

void StatusComponent::AddEvasionRate(float value)
{
	m_Status.evasionRate = std::clamp(m_Status.evasionRate + value, 0.0f, 0.8f);
}

void StatusComponent::AddSkillChainDamageRate(float value)
{
	ClampRate(m_Status.skillChainDamageRate, 1.0f, value);
}

void StatusComponent::AddSkillRangeRate(float value)
{
	ClampRate(m_Status.skillRangeRate, 0.1f, value);
}

void StatusComponent::AddSkillDurationRate(float value)
{
	m_Status.skillDurationRate += value;
}

float StatusComponent::CalculateSkillDuration(float baseDuration) const
{
	return baseDuration * m_Status.skillDurationRate;
}

