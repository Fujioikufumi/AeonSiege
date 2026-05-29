#include "HealthComponent.h"
#include "MathUtility.h"
#include "Scene.h"
#include "Camera.h"
#include <algorithm>
#include "imgui.h"

HealthComponent::HealthComponent(GameObject* obj)
	: Component(obj)
{
	m_ComponentName = "HealthComponent";
}

HealthComponent::~HealthComponent()
{
}

bool HealthComponent::Init()
{
	return true;
}

void HealthComponent::ApplyDamage(int damage)
{
	if (m_IsDead)
		return;
	m_HP -= damage;
	m_IsDamaged = true;
	if (m_HP <= 0)
	{
		m_HP = 0;
		m_IsDead = true;
	}
}

void HealthComponent::Heal(int amount)
{
	if (!m_IsDead)
		m_HP = std::min(m_HP + amount, m_MaxHP); // HP を回復（最大体力を超えないように）
}

void HealthComponent::SetMaxHP(int maxHp)
{
	// 最大HPの設定 1 ~ 99999の範囲に限定
	m_MaxHP = std::clamp(maxHp, kMinHp, kMaxHp);
	if (m_HP > m_MaxHP)
	{
		m_HP = m_MaxHP;
	}

}

void HealthComponent::SetHP(int hp)
{
	m_HP = std::clamp(hp, 0, m_MaxHP);
	// HP が 0 以下になった場合、死亡フラグを立てる
	if (m_HP <= 0)
	{
		m_IsDead = true;
	}
	else
	{
		m_IsDead = false; // HPが0より大きい場合は生存状態にする
	}
}

float HealthComponent::GetHPRatio() const
{
	if (m_MaxHP <= 0 || m_IsDead)
		return 0.0f;
	return static_cast<float>(m_HP) / static_cast<float>(m_MaxHP);
}

void HealthComponent::Serialize(nlohmann::json& json) const
{
	json["HP"] = m_HP;
	json["MaxHP"] = m_MaxHP;
}

void HealthComponent::Deserialize(const nlohmann::json& json)
{
	if (json.contains("MaxHp") && json["MaxHp"].is_number_integer())
		SetMaxHP(json["MaxHp"].get<int>());
	if (json.contains("HP") && json["HP"].is_number_integer())
		SetHP(json["HP"].get<int>());
}

void HealthComponent::OnDebugWindow()
{
	int maxHp = m_MaxHP;
	if (ImGui::DragInt("Max HP", &maxHp, 1.0f, kMinHp, kMaxHp))
	{
		SetMaxHP(maxHp);
	}
	int hp = m_HP;
	const int hpMaxForDrag = std::max(kMinHp, m_MaxHP);
	if (ImGui::DragInt("Current HP", &hp, 1.0f, 0, hpMaxForDrag))
	{
		SetHP(hp);
	}
	ImGui::Separator();
	ImGui::Text("HP ratio: %.2f", static_cast<double>(GetHPRatio()));
	ImGui::TextUnformatted(m_IsDead ? "State: Dead" : "State: Alive");
}
