#pragma once
#include "GameObject.h"
#include "StatusComponent.h"
#include "SkillData.h"
#include "DamageTypes.h"

struct AllyAIParams;
class AllyAIComponent;

enum class PaladinState
{
	Idle,
	Run,
	AutoAttack,
	GuardStart,
	GuardIdle,
	GuardImpact,
	SkillAttack,
	Dead
};

class Paladin : public GameObject
{
public:
	Paladin();
	~Paladin() override;

	bool Init() override;
	void Term() override;
	void Update(float deltaTime = 0.0f) override;

	/// <summary>
	/// 現在ガード状態（被ダメージ軽減中）かどうかを判定します。
	/// </summary>
	/// <returns>ガード中であれば true、それ以外は false</returns>
	bool IsGuarding() const;

	// Paladinが受けるダメージ処理。ガード状態やスキル攻撃の状態に応じてダメージを軽減など
	DamageResult ApplyDamage(const DamageContext& context) override;
private:
	AllyAIParams CreateAIParams() const;

	/// <summary>
	/// パラディンの状態を更新します。状態が変化した場合は、対応するアニメーションに切り替えます。
	/// </summary>
	/// <param name="newState">新しい状態（PaladinState）</param>
	void ChangeState(PaladinState newState);

	/// <summary>
	/// アニメーションを変更します。アニメーションコントローラーが設定されていない場合は処理をスキップします。
	/// </summary>
	/// <param name="anim">アニメーションコントローラーのポインタ</param>
	/// <param name="clipName">再生するアニメーションクリップの名前</param>
	/// <param name="loop">ループ再生するかどうか</param>
	/// <param name="speed">再生速度（デフォルトは1.0f）</param>
	void ChangeAnimation(class AnimationController* anim, const char* clipName, bool loop, float speed = 1.0f);

	/// <summary>
	/// プレイヤーを追従するための更新処理。プレイヤーから一定以上離れている場合は追従を開始し、近づいたら停止します。また、プレイヤーが停止している場合は待機位置を計算して移動します。
	/// </summary>
	/// <param name="deltaTime">前フレームからの経過時間（秒）</param>
	/// <param name="pScene">シーンへのポインタ</param>
	void UpdateFollowPlayer(float deltaTime, class Scene* pScene);

	/// <summary>
	/// 基本的な戦闘・索敵ロジックの更新処理。
	/// </summary>
	bool UpdateCombat(float deltaTime, class Scene* pScene);
	
	/// <summary>
	/// ガード処理の更新。ガード時間の管理などを行います。
	/// </summary>
	bool UpdateGuard(float deltaTime, class Scene* pScene);

	/// <summary>
	/// 状況に応じてガードの開始を試みます。
	/// </summary>
	bool TryStartGuard(class Scene* pScene);

	/// <summary>
	/// 現在のステートがガード関係のステートかどうかを判定します。
	/// </summary>
	bool IsGuardState() const;

	/// <summary>
	/// スキル攻撃（特殊攻撃）中の更新処理。ダメージ発生遅延などを管理します。
	/// </summary>
	bool UpdateSkillAttack(float deltaTime, class Scene* pScene);

	/// <summary>
	/// スキル攻撃の開始を試みます（クールダウンが明けている場合など）。
	/// </summary>
	bool TryStartSkillAttack(class Scene* pScene);

private:
	StatusComponent* m_Status = nullptr; // ステータス管理コンポーネント
	StatusData CreateStatusData() const;

	class AnimationController* m_Animation = nullptr; // アニメーション管理コンポーネント
	class AllyAIComponent* m_AllyAI = nullptr; // 味方AI管理コンポーネント

	float GetMoveSpeed() const;
	PaladinState m_State = PaladinState::Idle; // 現在のステート

	//-------------------------------------------------------------------
	// スキル関連
	SkillData CreateGuardSkillData() const;
	SkillData CreateSkillAttackData() const;
	SkillData m_GuardSkill;
	SkillData m_SkillAttack;

	// スキル攻撃用変数
	float m_SkillAttackCooldown = 0.0f;        // スキル攻撃のクールダウンタイマー
	float m_SkillDamageDelayTimer = 0.0f;      // スキルダメージ発生までの遅延タイマー
	bool m_HasPendingSkillDamage = false;      // スキルダメージの発生待ちフラグ

	// スキル攻撃関連定数
	static constexpr float kSkillAttackRange = 40.0f;       // スキル攻撃の射程
	static constexpr float kSkillAttackCooldownSec = 6.0f;  // スキル攻撃のクールダウン時間（秒）
	static constexpr float kSkillAttackDamageDelay = 0.45f; // スキル攻撃のダメージ発生までの遅延時間（秒）

	// 通常攻撃用変数
	GameObject* m_AttackTarget = nullptr;   // 現在攻撃しているターゲット
	float m_AutoAttackCooldown = 0.0f;      // 通常攻撃のオートアタック間隔タイマー
	float m_DamageDelayTimer = 0.0f;        // 通常攻撃のダメージ発生までの遅延タイマー
	bool m_HasPendingDamage = false;        // 通常攻撃のダメージ発生待ちフラグ

	// ガード関係変数・定数
	float m_GuardTimer = 0.0f;              // ガード中の経過時間タイマー
	float m_GuardCooldown = 0.0f;           // 次にガード可能になるまでのクールダウンタイマー
	static constexpr float kGuardDamageRate = 0.5f; // ガード時の受けるダメージの倍率（50%カット）

	static constexpr float kTargetKeepRange = 220.0f; // ターゲットを維持できる最大距離

	static constexpr float kGuardDuration = 3.0f;      // ガードの持続時間（秒）
	static constexpr float kGuardCooldownSec = 8.0f;   // ガードのクールダウン時間（秒）
	static constexpr float kGuardTriggerRange = 100.0f;// ガードをトリガーする敵との距離

	// その他の定数（索敵・攻撃・追従など）
	static constexpr float kEnemyDetectRange = 160.0f;       // 敵の索敵範囲
	static constexpr float kAutoAttackRange = 35.0f;         // 通常攻撃の射程
	static constexpr float kAutoAttackInterval = 1.8f;       // 通常攻撃を行う間隔（秒）
	static constexpr float kAutoAttackDamageDelay = 0.35f;   // 通常攻撃のダメージ発生までの遅延時間（秒）
	static constexpr int kAutoAttackDamage = 12;             // 通常攻撃の基本ダメージ

	static constexpr float kPlayerMoveThreshold = 0.05f;           // プレイヤーが動いていると判定する移動量の閾値
	static constexpr float kWaitTargetForwardDistance = 35.0f;     // 待機時のプレイヤー前方へのオフセット距離
	static constexpr float kWaitTargetSideOffset = 20.0f;          // 待機時のプレイヤー横方向へのオフセット距離
	static constexpr float kWaitTargetArriveDistance = 3.0f;       // 待機目標に到達したと判定する距離
	static constexpr float kMaxWaitTargetDistanceFromPlayer = 60.0f; // プレイヤーから待機目標までの許容最大距離

	static constexpr float kFollowStartDistance = 60.0f; // 追従を開始するプレイヤーとの距離
	static constexpr float kFollowStopDistance = 45.0f;  // 追従を停止するプレイヤーとの距離
	static constexpr float kFollowMoveSpeed = 45.0f;     // 追従時の移動速度
	static constexpr float kTurnSpeed = 10.0f;           // 振り向きの速度

	// アニメーションクリップ名
	static constexpr const char* kAnimIdle = "Paladin_Idle";
	static constexpr const char* kAnimRun = "Paladin_Run";
	static constexpr const char* kAnimSlash = "Paladin_Slash";
	static constexpr const char* kAnimBlock = "Paladin_Block";
	static constexpr const char* kAnimBlockIdle = "Paladin_Block_Idle";
	static constexpr const char* kAnimBlockImpact = "Paladin_Block_Impact";
	static constexpr const char* kAnimAttack = "Paladin_Attack";
	static constexpr const char* kAnimDeath = "Paladin_Death";
};