#pragma once
#include "Component.h"

/// <summary>
///	体力管理コンポーネント
/// </summary>
class HealthComponent : public Component
{
public:
	static constexpr int kMaxHp = 99999;
	static constexpr int kMinHp = 1;

	HealthComponent(GameObject* obj);
	virtual ~HealthComponent() override;

	using Component::Init;
	bool Init() override;

	// ダメージを与える
	void ApplyDamage(int damage);

	/// 回復を適用
	void Heal(int amount);

	/// 最大HPを設定
	void SetMaxHP(int maxHp);

	/// 現在のHPを設定
	void SetHP(int hp);

	/// 現在のHPを取得	
	[[nodiscard]] int GetHP() const { return m_HP; }

	/// 最大HPを取得
	[[nodiscard]] int GetMaxHP() const { return m_MaxHP; }
	
	/// HPの割合を取得
	[[nodiscard]] float GetHPRatio() const;

	/// 生存しているかどうか
	[[nodiscard]] bool IsAlive() const { return !m_IsDead; }

	// ダメージ受けたかどうか
	[[nodiscard]] bool IsDamaged() const { return m_IsDamaged; }

	void Serialize(nlohmann::json& json) const override;
	void Deserialize(const nlohmann::json& json) override;
	void OnDebugWindow() override;
private:
	int m_HP = 100;				// 現在の体力
	int m_MaxHP = 100;			// 最大体力
	bool m_IsDead = false;		// 死亡フラグ
	bool m_IsDamaged = false;	// ダメージを受けたフラグ（HP が減ったフレームのみ true）
};