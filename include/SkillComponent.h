#pragma once
#include "Component.h"
#include "SkillData.h"
#include <vector>

/// <summary>
/// スキルの発動時のデータ構造体
/// </summary>
struct SkillRuntime
{
	SkillData Data;				  // スキルの基本データ
	float CurrentCooldown = 0.0f; // 現在のクールダウン時間
};

/// <summary>
/// スキルコンポーネント：スキルのクールダウン管理を担当
/// </summary>
class SkillComponent : public Component
{
public:
	SkillComponent(GameObject* owner);
	~SkillComponent() override = default;

	void Update(float deltaTime) override;

	// スキルを追加
	void AddSkill(const SkillData& data);

	// 指定したスキルIDを持っているかの確認
	[[nodiscard]] bool HasSkill(SkillId id) const;

	// 指定したスキルスロットのスキルデータを取得
	[[nodiscard]] const SkillRuntime* GetSkillBySlot(int slotIndex) const;
	[[nodiscard]] SkillRuntime* GetSkillBySlot(int slotIndex);

	// 指定したスキルスロットのクールダウンを開始
	void StartCooldown(int slotIndex, float cooldownSec);

	// 指定したスキルのクールダウン比率を取得（0.0f～1.0f）
	[[nodiscard]] float GetCooldownRatio(int slotIndex) const;

	// 現在保持しているスキルの数を取得
	[[nodiscard]] int GetSkillCount() const { return static_cast<int>(m_Skills.size()); }

private:
	// クールダウンの更新
	void UpdateCooldowns(float deltaTime);

	std::vector<SkillRuntime> m_Skills;		// スキルデータのリスト
	static constexpr int kMaxSkillSlot = 6;
};