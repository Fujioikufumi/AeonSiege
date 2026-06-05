#pragma once
#include "Component.h"
#include <DirectXMath.h>
#include <string>
#include <functional>

// 敵のAIステート
enum class EnemyAIState
{
	Idle,	// 待機
	Patrol, // 巡回
	Chase,	// 追跡
	Attack, // 攻撃
	Dead	// 死亡
};

class EnemyAIComponent : public Component
{
public:
	EnemyAIComponent(GameObject* pObj);
	virtual ~EnemyAIComponent() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	//---------------------------------------------------------
	// AIのパラメータ設定
	//---------------------------------------------------------

	/// 敵のレベルを設定
	void SetLevel(int level) { m_Level = level; }

	/// 敵のレベルを取得
	int GetLevel() const { return m_Level; }

	/// 敵のスポーン位置を設定
	void SetSpawnPosition(const DirectX::XMFLOAT3& pos) { m_SpawnPosition = pos; }

	/// 徘徊する半径を設定
	void SetPatrolRadius(float radius) { m_PatrolRadius = radius; }

	/// プレイヤーを発見する索敵半径を設定
	void SetAggroRadius(float radius) { m_AggroRadius = radius; }

	/// 徘徊するかどうかを設定
	void SetUsePatrol(bool usePatrol) { m_UsePatrol = usePatrol; }

	/// 常に敵対状態にするかどうかを設定
	void SetAlwaysAggro(bool alwaysAggro) { m_AlwaysAggro = alwaysAggro; }

	/// 攻撃射程を設定
	void SetMeleeRange(float range) { m_MeleeRange = range; }

	/// 移動速度を設定
	void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

	/// 追跡時の移動速度を設定
	void SetChaseMoveSpeed(float speed) { m_ChaseMoveSpeed = speed; }

	/// 攻撃のクールダウン時間を設定(現状通常攻撃のみ)
	void SetMeleeCooldownSec(float sec) { m_MeleeCooldownSec = sec; }

	// AIの有効/無効を設定
	void SetAIActive(bool active) { m_IsAIActive = active; }
	bool IsAIActive() const { return m_IsAIActive; }

public:
	// EnemyAIStateに応じたアニメーション再生のためのコールバックと、
	// 攻撃発生時の処理のためのコールバックを設定
	using AnimCallback = std::function<void(EnemyAIState)>;
	void SetAnimCallback(AnimCallback cb) { m_AnimCallback = cb; }
	using AttackCallback = std::function<void(GameObject*)>;
	void SetAttackCallback(AttackCallback cb) { m_AttackCallback = cb; }

	// 経験値報酬を設定
	void SetExpReward(int value) { m_ExpReward = value; }
	int GetExpReward() const { return m_ExpReward; }
private:
	// 待機状態の更新
	void UpdateIdle(float deltaTime);

	// 徘徊状態の更新
	void UpdatePatrol(float deltaTime);

	// 追跡状態の更新
	void UpdateChaseAndAttack(float deltaTime, class GameObject* player);

	// 攻撃状態の更新
	GameObject* FindAttackTarget(class Scene* pScene) const;

	// 状態遷移処理
	void ChangeState(EnemyAIState newState);
private:
	static constexpr float kIdleWaitTime = 3.0f; // 待機状態での待ち時間

	EnemyAIState m_CurrentState = EnemyAIState::Idle;

	bool m_IsAIActive = true;

	int m_Level = 1; // 敵のレベル
	int m_ExpReward = 0; // 撃破されたらプレイヤーが得る経験値量
	DirectX::XMFLOAT3 m_SpawnPosition = { 0, 0, 0 }; // 初期座標
	DirectX::XMFLOAT3 m_TargetPatrolPos = { 0, 0, 0 }; // 次の徘徊目的地

	float m_PatrolRadius = 30.0f; // 徘徊範囲
	float m_AggroRadius = 60.0f;  // 索敵範囲
	float m_MeleeRange = 25.0f;   // 攻撃射程

	float m_MoveSpeed = 10.0f;      // 通常移動速度
	float m_ChaseMoveSpeed = 25.0f; // 追跡時の移動速度

	float m_StateTimer = 0.0f;    // 待機時間などを計る汎用タイマー
	float m_MeleeCooldown = 0.0f; // 攻撃のクールダウン
	float m_MeleeCooldownSec = 4.0f;

	bool m_IsAttacking = false; // 攻撃中かどうかのフラグ

	bool m_UsePatrol = true; // 徘徊するかどうかのフラグ
	bool m_AlwaysAggro = false; // 常に敵対状態にするかどうかのフラグ（trueの場合、プレイヤーが近くになくても常に追跡状態になる）

	AnimCallback m_AnimCallback;      // 状態に応じたアニメーション再生用
	AttackCallback m_AttackCallback;  // 攻撃発生時の処理（ダメージ適用）
};