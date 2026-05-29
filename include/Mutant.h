#pragma once
#include "GameObject.h"
#include "StatusComponent.h"

class Terrain;
enum class EnemyAIState;

// ジャンプ攻撃スキルの内部状態
enum class MutantJumpSkillState
{
	Disabled,         // HP > 50%
	Cooldown,         // エンレージ中・CD 回転中（通常攻撃は可能）
	WaitCurrentAnim,  // CD 完了・Swiping 終了待ち
	Executing,        // ジャンプアニメ再生中
};

class Mutant : public GameObject
{
public:
	Mutant();
	~Mutant() override;
	bool Init() override;
	void Term() override;
	void Update(float deltaTime = 0.0f) override;

private:
	void OnAIStateChanged(EnemyAIState newState);
	void OnAIAttack(GameObject* target);
	void ChangeAnimation(class AnimationController* anim, const char* clipName, bool loop, float speed = 1.0f);

	// 通常近接
	void UpdatePendingMeleeDamage(float deltaTime);
	void CancelPendingMeleeDamage();

	//-----------------------------------------------
	// ジャンプ攻撃スキル

	// エンゲージ状態の判定（HP 60%以下でエンレージ）
	bool IsEnraged() const;
	
	// ジャンプ攻撃が現在のアニメーション状態で使用可能かどうかの判定
	bool CanStartJumpAttackNow(const class AnimationController* anim) const;
	
	// ジャンプ攻撃の開始
	void StartJumpAttack();

	// ジャンプ攻撃の更新
	void UpdateJumpAttackSkill(float deltaTime);
	
	// ジャンプ攻撃の実行（ダメージ判定と効果適用）
	void UpdateJumpAttackExecution(float deltaTime);
	
	// ジャンプ攻撃のダメージを全ての味方に適用
	void ApplyJumpAttackDamageToAllAllies();
	
	// ジャンプ攻撃の終了処理（アニメーションのリセットやフラグのクリアなど）
	void FinishJumpAttack();
	
	// ジャンプ攻撃のアニメーションを一時停止
	void ResumeAutoAttackAnimation();
	
	bool m_JumpDamageApplied = false;
private:
	class HealthComponent* m_pHealth = nullptr;
	StatusComponent* m_Status = nullptr;

	static constexpr const char* kAnimIdle = "Mutant_Normal_Idle";
	static constexpr const char* kAnimWalk = "Mutant_Normal_Walk";
	static constexpr const char* kAnimRun = "Mutant_Combat_Run";
	static constexpr const char* kAnimSwiping = "Mutant_Combat_Swiping";
	static constexpr const char* kAnimDying = "Mutant_Combat_Dying";
	static constexpr const char* kAnimJumpAttack = "Mutant_Combat_JumpAttack";

	static constexpr float kMeleeHitRange = 25.0f;
	bool m_HasPendingMeleeDamage = false;
	float m_MeleeDamageDelayTimer = 0.0f;
	GameObject* m_PendingMeleeTarget = nullptr;
	int m_PendingMeleeDamage = 0;


	MutantJumpSkillState m_JumpSkillState = MutantJumpSkillState::Disabled;
	bool m_WasEnraged = false;
	float m_JumpAttackCooldownTime = 0.0f;
	float m_JumpLandTimer = 0.0f;


	static constexpr float kTargetHeightOffset = 10.0f;   // ターゲットの中心からどれだけ上を狙うか（ジャンプ攻撃の着地点の高さ調整）
	static constexpr float kMeleeRange = 25.0f;           // AI射程 = ダメージ判定距離
	static constexpr float kChaseMoveSpeed = 38.0f;		  // 追跡移動速度
	static constexpr float kMeleeCooldownSec = 3.0f;      // 自動攻撃間隔（AI CD）
	static constexpr float kMeleeDamageDelaySec = 1.4f;   // Swiping ヒットタイミング
	// --- ジャンプ攻撃 ---
	static constexpr float kEnrageHPRatio = 0.6f;		  // エンレージ HP 比率
	static constexpr float kJumpAttackCooldownSec = 6.0f; // ジャンプ攻撃のクールダウン時間
	static constexpr float kJumpAttackDamageDelaySec = 1.6f; // ジャンプ攻撃のダメージ判定が発生するまでの時間（ジャンプ攻撃アニメーションのヒットタイミングに合わせる）
	static constexpr float kJumpAttackDamageRate = 1.5f;	 // ジャンプ攻撃のダメージ倍率（基本攻撃力に対する倍率）
};