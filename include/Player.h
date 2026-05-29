#pragma once
#include "GameObject.h"
#include "Scene.h"
#include "StatusComponent.h"
#include "SkillComponent.h"
#include "AnimationController.h"
#include "SkillData.h"
#include <vector>

class Camera;
class Terrain;

enum class PlayerState
{
	Idle,
	Walk,
	Run,
	Jump,
	Combat,
};

/// <summary>
/// プレイヤーキャラクターの制御やステータス・戦闘入力の管理を行うクラス。
/// </summary>
class Player : public GameObject
{
public:
	Player();
	~Player();

	bool Init() override;
	void Term() override;
	void Update(float deltaTime) override;

	[[nodiscard]] XMVECTOR GetQuaternion() const { return m_Quaternion; }
	[[nodiscard]] XMMATRIX GetWorldMatrix() const override;
	[[nodiscard]] XMFLOAT3 GetForward() const override;

	[[nodiscard]] const GameObject* GetLockTarget() const { return m_LockTarget; }
	[[nodiscard]] bool IsInCombat() const { return m_IsCombat; }
	[[nodiscard]] bool IsDead() const { return m_IsDead; } // Player独自の死亡フラグ
	[[nodiscard]] bool IsDeadAnimFinished() const { return m_IsDeadAnimFinished; } // 死亡アニメーションが終了したかどうか

	[[nodiscard]] float GetSkillCooldownRatioBySlot(int slotIndex) const;
private:
	//-----------------------------------------------------------
	// ステータス関連
	[[nodiscard]] StatusData CreateStatusData() const;
	StatusComponent* m_Status = nullptr;
	static constexpr int kDefaultPlayerHP = 100;
	bool m_IsCombat = false; // 現在戦闘中かどうか
	bool m_IsDead = false; // Player独自の死亡フラグ
	bool m_IsDeadAnimFinished = false; // 死亡アニメーションが終了したかどうか
	//-----------------------------------------------------------
	// 戦闘入力管理
	float m_AHoldTime = 0.0f;
	bool m_WasAHeld = false;
	bool m_ExitCombat = false; // 長押し操作で、既に戦闘状態から抜ける入力があったかどうか

	//-----------------------------------------------------------
	// 戦闘アニメーション制御
	bool m_PlayingCombatAnim = false;
	AnimationController* m_AnimController = nullptr;

	void StartCombatAnim(const char* clipName, float speed = 1.0f);
	void UpdateCombatAnim();
	void ResumeLocomotionAnim();

	//-----------------------------------------------------------
	// ロックオンシステム

	// 視界の判定 ※ yは考慮していない
	[[nodiscard]] bool IsInSight(const Camera* camera, const GameObject* enemy) const;

	// ロックオンターゲットの更新
	void UpdateLockOnTarget(float deltaTime, Scene* scene, Camera* camera);

	// ロックオンターゲットが有効かどうかの確認
	void CheckLockTarget(Scene* scene);

	// ロックオン入力の処理
	void HandleLockOnInput(float deltaTime, Scene* scene, Camera* camera);

	// 戦闘の開始・終了の入力判定
	void HandleCombatInput(float deltaTime, Scene* scene, Camera* camera);

	// ロックオン対象の候補を収集
	[[nodiscard]] std::vector<GameObject*> CollectLockCandidates(const Scene* scene, const Camera* camera) const;

	// ロックオン対象の切り替え direction: 1 = 次, -1 = 前
	void SwitchTarget(const Scene* scene, const Camera* camera, int direction);

	GameObject* m_LockTarget = nullptr; // 現在のロックオン対象
	static constexpr float kYHoldLimit = 0.8f;
	float m_YHoldTime = 0.0f;
	bool  m_WasYHeld = false;

	//-----------------------------------------------------------
	// オートアタック関連
	void UpdateAutoAttack(float deltaTime);

	float m_CurrentAutoAtkCd = 0.0f;
	static constexpr float kAutoAttackInterval = 1.75f; // オートアタックの発動間隔（秒）
	static constexpr float kAutoAttackRange = 50.0f;	// オートアタックの射程距離
	static constexpr int kAttackDamage = 15;			// オートアタックの基本ダメージ
	static constexpr float kAttackDamageDelay = 0.5f;	// オートアタックのダメージ発生の遅延時間（秒）
	float m_DamageDelayTime = 0.0f; // ダメージの遅延時間（秒）。攻撃アニメーションとダメージ発生のタイミングを合わせるために使用
	int m_PendingDamage = 0;		// 発生予定のダメージ量。

	//-----------------------------------------------------------
	// スキル関連

	void UpdateSkills(float deltaTime);

	void UpdateSkillState(float deltaTime);

	// 入力されたスキルが発動可能かどうかを判定する
	bool TryStartSkillSlot(int slotIndex);

	// 発動中のスキルの更新処理
	void UpdateActiveSkill(float deltaTime);

	// 発動中のスキルのヒット処理
	void ApplyActiveSkillHit();

	// スキルのダメージをスキルの対象に適用する
	bool ApplySkillDamage(GameObject* target, const DamageContext& context);
	
	// スキルのダメ―ジを与える対象を収集する
	std::vector<GameObject*> CollectSkillTargets(const SkillData& skill) const;
	
	// ターゲットがスキルの範囲内にいるかどうかを判定する
	bool IsInForwardCone(const GameObject* target, float range, float angleDeg) const;

	// 遅延ダメージの更新処理
	void UpdateDamageDelay(float deltaTime, Camera* camera);

	static constexpr float kChainSec = 0.6f;	// チェインの猶予時間（秒）
	float	m_CurrentChainTime = 0.0f;			// 現在のチェイン判定のカウントダウン
	float	m_ActiveSkillDamageRate = 1.0f;		// 現在のスキル攻撃に対するチェインダメージの倍率。
	bool	m_HasActiveSkill = false;			// 現在スキルが発動中かどうか
	SkillData	m_ActiveSkill;					// 発動中のスキルのデータ
	GameObject* m_SkillTarget = nullptr;		// スキルの対象（単体攻撃の場合）
	float	m_ActiveSkillTime = 0.0f;			// 発動中のスキルの経過時間
	int		m_ActiveSkillHitCount = 0;			// 発動中のスキルのヒット回数

	// スキル攻撃やオートアタックのダメージ計算と適用
	void CalculateAttackDamage(bool isAutoAttack, Camera* camera = nullptr);

	//-----------------------------------------------------------
	// プレイヤー制御
	
	// 戦闘状態の更新
	void UpdateCombatMode(float deltaTime, Scene* scene, Camera* camera, Terrain* terrain);
	
	// 移動の更新
	void UpdateMovement(float deltaTime, Camera* camera, Terrain* terrain);

	// カメラの注視点をプレイヤーの位置に合わせる
	void SyncCameraTarget(Camera* camera);

	XMVECTOR m_Quaternion = XMQuaternionIdentity();
	float m_VelocityY = 0.0f;
	bool m_IsGrounded = true;
	//-----------------------------------------------------------
	// アニメーション制御
	PlayerState m_State = PlayerState::Idle;				// プレイヤーの現在の状態（ステート）
	PlayerState m_PrevState = PlayerState::Idle;			// 1フレーム前のプレイヤーの状態
	void ChangeAnimation();

	// アニメーションのクリップ名
	static constexpr const char* kAnimIdle = "Idle";		// 待機
	static constexpr const char* kAnimWalk = "Walk";		// 歩き
	static constexpr const char* kAnimRun = "Run";			// 走り
	static constexpr const char* kAnimJump = "Jump";		// ジャンプ
	static constexpr const char* kAnimAttackAuto = "Player_Attack01";	// 自動攻撃
	static constexpr const char* kAnimCombatIdle = "Player_Combat_Idle";	// 戦闘中の待機

	// 移動速度設定
	static constexpr float kMoveSpeed = 15.0f;	// 基本移動速度
	static constexpr float kRunMoveRate = 2.0f;	// 走り状態の速度倍率

	//-----------------------------------------------------------
	// ヒットエフェクト
	void TriggerEnemyHitEffect(Camera* camera, float hitHoldSec, float shakeMag, float shakeDurSec);
	void UpdateHitEffect(float deltaTime);

	float m_HitPause = 0.0f;								// Pause 維持時間（秒）
	static constexpr float kEnemyHitImpactHoldAuto = 0.05f;
	static constexpr float kEnemyHitImpactHoldSkill = 0.06f;
	static constexpr float kEnemyHitShakeMag = 0.12f;
	static constexpr float kEnemyHitShakeDuration = 0.51f;
};