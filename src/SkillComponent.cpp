#include "SkillComponent.h"
#include "GameObject.h"
#include <algorithm>

SkillComponent::SkillComponent(GameObject* owner)
	: Component(owner)
{
	m_ComponentName = "SkillComponent";
}

void SkillComponent::Update(float deltaTime)
{
	UpdateCooldowns(deltaTime);
}

void SkillComponent::AddSkill(const SkillData& data)
{
	// スキルIDがNoneでないことと、すでに同じスキルを持っていないことを確認
	if (data.id == SkillId::None || HasSkill(data.id))
	{
		return;
	}

	if (static_cast<int>(m_Skills.size()) >= kMaxSkillSlot)
	{
		return; // 最大スロット数に達している
	}

	SkillRuntime skill;
	skill.Data = data;
	skill.CurrentCooldown = 0.0f;
	m_Skills.push_back(skill);
}

bool SkillComponent::HasSkill(SkillId id) const
{
	// スキルIDが一致するものがあるかどうかを確認
	return std::any_of(m_Skills.begin(), m_Skills.end(),
		[id](const SkillRuntime& skill) { return skill.Data.id == id; });
}

const SkillRuntime* SkillComponent::GetSkillBySlot(int slotIndex) const
{
	if (slotIndex < 0 || static_cast<size_t>(slotIndex) >= m_Skills.size())
	{
		return nullptr;
	}
	return &m_Skills[slotIndex];
}

SkillRuntime* SkillComponent::GetSkillBySlot(int slotIndex)
{
	if (slotIndex < 0 || static_cast<size_t>(slotIndex) >= m_Skills.size())
	{
		return nullptr;
	}
	return &m_Skills[slotIndex];
}

void SkillComponent::StartCooldown(int slotIndex, float cooldownSec)
{
	SkillRuntime* skill = GetSkillBySlot(slotIndex);
	if (skill)
	{
		skill->CurrentCooldown = cooldownSec;
	}
}

void SkillComponent::UpdateCooldowns(float deltaTime)
{
	for (SkillRuntime& skill : m_Skills)
	{
		if (skill.CurrentCooldown > 0.0f)
		{
			skill.CurrentCooldown = max(0.0f, skill.CurrentCooldown - deltaTime);
		}
	}
}

float SkillComponent::GetCooldownRatio(int slotIndex) const
{
	const SkillRuntime* skill = GetSkillBySlot(slotIndex);
	if (!skill || skill->Data.cooldownSec <= 0.0f)
	{
		return 0.0f;
	}

	return skill->CurrentCooldown / skill->Data.cooldownSec;
}