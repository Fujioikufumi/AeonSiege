#pragma once
#include "GameObject.h"
#include "StatusComponent.h"

class Terrain;
enum class EnemyAIState;

enum class ZombieType
{
	Normal,
	SkeletonZombie,
};

enum class ZombieAttackType
{
	Punch,
	Kick,
};

/// <summary>
/// ゾンビクラス
/// </summary>
class Zombie : public GameObject
{
public:
	Zombie();
	~Zombie() override;

	bool Init() override;
	void Term() override;
	void Update(float deltaTime = 0.0f) override;

protected:
	explicit Zombie(ZombieType type);

private:
	// EnemyAIComponent から呼び出されるステート変化・攻撃イベントのハンドラー
	void OnAIStateChanged(EnemyAIState newState);
	void OnAIAttack(GameObject* target);

	void SelectAttackType();
	int GetAttackDamage() const;

	void ChangeAnimation(class AnimationController* anim, const char* clipName, bool loop, float speed = 1.0f);

private:
	static constexpr const char* kAnimRun = "Zombie_Run";
	static constexpr const char* kAnimAttack = "Zombie_Attack";
	static constexpr const char* kAnimKick = "Zombie_Kick";
	static constexpr const char* kAnimDying = "Zombie_Dying";

	ZombieType m_Type = ZombieType::Normal; // ゾンビの種類
	ZombieAttackType m_CurrentAttackType = ZombieAttackType::Punch; // 現在の攻撃タイプ

	StatusComponent* m_Status = nullptr;

	// 近接攻撃のヒットをアニメに合わせて遅延させる（Paladin の pending パターン）
	void UpdatePendingMeleeDamage(float deltaTime);
	void CancelPendingMeleeDamage();
	static constexpr float kMeleeDamageDelaySec = 1.0f; // アニメの当たり付近に合わせて調整
	static constexpr float kMeleeHitRange = 22.0f;      // Init の SetMeleeRange と一致させる
	static constexpr float kKickAttackRate = 35.0f; // キック攻撃の発動確率
	static constexpr float kKickDamageRate = 2.0f; // キックの威力倍率
	bool m_HasPendingMeleeDamage = false;
	float m_MeleeDamageDelayTimer = 0.0f;
	GameObject* m_PendingMeleeTarget = nullptr;
	int m_PendingMeleeDamage = 0;
};