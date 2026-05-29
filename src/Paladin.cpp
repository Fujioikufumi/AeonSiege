#include "Paladin.h"
#include "MeshRenderer.h"
#include "Terrain.h"
#include "Scene.h"
#include "GameManager.h"
#include "HealthComponent.h"
#include "AnimationController.h"
#include "TargetComponent.h"
#include "MathUtility.h"
#include "GameFlowUtil.h"
#include "AllyAIComponent.h"
#include "StatusComponent.h"

Paladin::Paladin()
	: GameObject()
{
	m_ModelPath = L"../Assets/Characters/Paladin.bmdl";
	m_Scale = { 0.05f, 0.05f, 0.05f };
}

Paladin::~Paladin()
{
	GameObject::Term();
}

bool Paladin::Init()
{
	GameObject::Init();

	// モデル描画
	AddComponent<MeshRenderer>()->Load(m_ModelPath, m_PipelineName);

	// ステータスコンポーネントの追加
	m_Status = AddComponent<StatusComponent>();
	m_Status->Setup(CreateStatusData());

	// 体力コンポーネントの追加
	HealthComponent* health = AddComponent<HealthComponent>();
	health->SetMaxHP(m_Status->GetMaxHp());
	health->SetHP(m_Status->GetMaxHp());

	// 味方AIコンポーネントの追加
	m_AllyAI = AddComponent<AllyAIComponent>();
	m_AllyAI->Setup(CreateAIParams());

	// ターゲットコンポーネントの追加
	TargetComponent* target = AddComponent<TargetComponent>();
	target->SetHeightOffset(10.0f);

	// アニメーションコントローラーの追加
	m_Animation = AddComponent<AnimationController>();
	m_Animation->SetLoop(true);
	m_Animation->Play(kAnimIdle);

	// ガードスキル
	m_GuardSkill = CreateGuardSkillData();
	// 攻撃スキル
	m_SkillAttack = CreateSkillAttackData();

	// 初期位置を地形の高さに合わせる
	Scene* pScene = GameManager::GetScene();
	if (pScene)
	{
		Terrain* pTerrain = pScene->GetGameObjectByName<Terrain>("Terrain");
		if (pTerrain)
		{
			m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);
		}
	}

	return true;
}

void Paladin::Term()
{
	GameObject::Term();
}

void Paladin::Update(float deltaTime)
{
	if (GameFlowUtil::IsShopOpen())
	{
		
		m_AllyAI->ClearAttackTarget();
		m_AttackTarget = nullptr;
		GameObject::Update(deltaTime);
		return;
	}

	Scene* pScene = GameManager::GetScene();
	if (pScene == nullptr) return;
	HealthComponent* pHealth = GetComponent<HealthComponent>();

	if (pHealth == nullptr || !pHealth->IsAlive())
	{
		//---------------------------------------------------------------------
		// 死亡処理
		if (m_State != PaladinState::Dead)
		{
			ChangeState(PaladinState::Dead);
		}
		if (m_Animation != nullptr && m_Animation->GetCurrentAnimationName() == kAnimDeath && !m_Animation->IsPlaying())
		{
			// 死亡しても削除しない
			return;
		}
		GameObject::Update(deltaTime);
		return;
	}

	if (GameFlowUtil::IsShopOpen())
	{
		GameObject::Update(deltaTime);
		return;
	}

	//---------------------------------------------------------------------
	// 生存中の処理

	if (!UpdateGuard(deltaTime, pScene))
	{
		if (!UpdateSkillAttack(deltaTime, pScene))
		{
			if (!UpdateCombat(deltaTime, pScene))
			{
				UpdateFollowPlayer(deltaTime, pScene);
			}
		}
	}
	Terrain* pTerrain = pScene->GetGameObjectByName<Terrain>("Terrain");
	if (pTerrain != nullptr)
	{
		m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);
	}
	GameObject::Update(deltaTime);
}

//-----------------------------------------------------------------------------
//	味方AIのパラメータを生成します。
//-----------------------------------------------------------------------------
AllyAIParams Paladin::CreateAIParams() const
{
	AllyAIParams params;
	params.followStartDistance = kFollowStartDistance;
	params.followStopDistance = kFollowStopDistance;
	params.waitTargetForwardDistance = kWaitTargetForwardDistance;
	params.waitTargetSideOffset = kWaitTargetSideOffset;
	params.waitTargetArriveDistance = kWaitTargetArriveDistance;
	params.maxWaitTargetDistanceFromPlayer = kMaxWaitTargetDistanceFromPlayer;
	params.enemyDetectRange = kEnemyDetectRange;
	params.targetKeepRange = kTargetKeepRange;
	params.moveSpeed = GetMoveSpeed();
	params.turnSpeed = kTurnSpeed;
	params.playerMoveThreshold = kPlayerMoveThreshold;
	return params;
}

//-----------------------------------------------------------------------------
// 	ガードスキルのデータを生成します。
//-----------------------------------------------------------------------------
SkillData Paladin::CreateGuardSkillData() const
{
	SkillData data;
	data.id = SkillId::PlayerSlash1;
	data.type = SkillType::Guard;
	data.animationName = kAnimBlock;
	data.cooldownSec = kGuardCooldownSec;
	data.range = kGuardTriggerRange;
	data.durationSec = kGuardDuration;
	return data;
}

//-----------------------------------------------------------------------------
// 	攻撃スキルのデータを生成します。
//-----------------------------------------------------------------------------
SkillData Paladin::CreateSkillAttackData() const
{
	SkillData data;
	data.id = SkillId::PlayerSlash2;
	data.type = SkillType::Attack;
	data.animationName = kAnimAttack;
	data.skillPowerRate = 2.0f;
	data.cooldownSec = kSkillAttackCooldownSec;
	data.range = kSkillAttackRange;
	data.effectDelaySec = kSkillAttackDamageDelay;
	return data;
}

void Paladin::UpdateFollowPlayer(float deltaTime, Scene* pScene)
{
	if (m_AllyAI == nullptr)
	{
		ChangeState(PaladinState::Idle);
		return;
	}
	const bool moved = m_AllyAI->UpdateFollowPlayer(deltaTime, pScene, GetMoveSpeed());
	ChangeState(moved ? PaladinState::Run : PaladinState::Idle);
}

bool Paladin::UpdateCombat(float deltaTime, Scene* pScene)
{
	if (pScene == nullptr) return false;

	// 自動攻撃のクールダウンを更新
	if (m_AutoAttackCooldown > 0.0f)
	{
		m_AutoAttackCooldown -= deltaTime;
	}

	// ダメージ発生の遅延を管理
	if (m_HasPendingDamage)
	{
		m_DamageDelayTimer -= deltaTime;

		if (m_DamageDelayTimer <= 0.0f)
		{
			//--------------------------------------------
			// ダメージ発生
			m_HasPendingDamage = false;

			if (m_AttackTarget != nullptr && !m_AttackTarget->IsDestroyed())
			{
				// ターゲットの体力コンポーネントを取得してダメージを適用
				HealthComponent* targetHp = m_AttackTarget->GetComponent<HealthComponent>();
				if (targetHp != nullptr && targetHp->IsAlive() &&
					MathUtility::IsInRange(m_Position, m_AttackTarget->GetPosition(), kAutoAttackRange))
				{
					DamageContext context;
					context.attacker = this;
					context.damage = m_Status->GetAttackPower();
					context.isCombo = false;
					context.isCritical = false;

					// 攻撃対象にダメージ情報を渡す。(AIなのでヒットストップなどはしない)
					m_AttackTarget->ApplyDamage(context);
				}
			}
		}
	}

	if (m_AllyAI == nullptr) return false;
	GameObject* enemy = m_AllyAI->GetOrFindAttackTarget(pScene);

	if (m_State == PaladinState::AutoAttack)
	{
		if (m_AttackTarget != nullptr && !m_AttackTarget->IsDestroyed())
		{
			HealthComponent* hp = m_AttackTarget->GetComponent<HealthComponent>();
			if (hp != nullptr && hp->IsAlive())
			{
				m_AllyAI->FaceTarget(deltaTime, m_AttackTarget->GetPosition());
			}
		}
		if (m_Animation != nullptr && m_Animation->GetCurrentAnimationName() == kAnimSlash && m_Animation->IsPlaying())
		{
			return true;
		}
		ChangeState(PaladinState::Idle);
		return true;
	}

	if (enemy == nullptr)
	{
		m_AllyAI->ClearAttackTarget();
		m_AttackTarget = nullptr;
		return false;
	}

	const float distance = MathUtility::CalculateDistanceXZ(m_Position, enemy->GetPosition());
	if (distance > kEnemyDetectRange)
	{
		m_AllyAI->ClearAttackTarget();
		m_AttackTarget = nullptr;
		return false;
	}

	m_AttackTarget = enemy;

	if (distance > kAutoAttackRange)
	{
		const bool moved = m_AllyAI->MoveToTargetPosition(deltaTime, enemy->GetPosition(), kAutoAttackRange, GetMoveSpeed());
		ChangeState(moved ? PaladinState::Run : PaladinState::Idle);
		return true;
	}

	m_AllyAI->FaceTarget(deltaTime, enemy->GetPosition());

	if (m_AutoAttackCooldown <= 0.0f)
	{
		m_HasPendingDamage = true;
		m_DamageDelayTimer = kAutoAttackDamageDelay;
		m_AutoAttackCooldown = kAutoAttackInterval;

		ChangeState(PaladinState::AutoAttack);
		return true;
	}

	ChangeState(PaladinState::Idle);
	return true;
}

bool Paladin::IsGuarding() const
{
	return m_State == PaladinState::GuardStart ||
		m_State == PaladinState::GuardIdle ||
		m_State == PaladinState::GuardImpact;
}

bool Paladin::IsGuardState() const
{
	return IsGuarding();
}

bool Paladin::UpdateGuard(float deltaTime, Scene* pScene)
{
	if (m_GuardCooldown > 0.0f)
	{
		m_GuardCooldown -= deltaTime;
	}

	if (m_State == PaladinState::GuardStart)
	{
		if (m_Animation != nullptr && m_Animation->GetCurrentAnimationName() == kAnimBlock && m_Animation->IsPlaying())
		{
			return true;
		}

		ChangeState(PaladinState::GuardIdle);
		return true;
	}

	if (m_State == PaladinState::GuardIdle)
	{
		m_GuardTimer -= deltaTime;

		if (m_GuardTimer <= 0.0f)
		{
			ChangeState(PaladinState::Idle);
			return false;
		}

		return true;
	}

	if (m_State == PaladinState::GuardImpact)
	{
		if (m_Animation != nullptr && m_Animation->GetCurrentAnimationName() == kAnimBlockImpact && m_Animation->IsPlaying())
		{
			return true;
		}

		if (m_GuardTimer > 0.0f)
		{
			ChangeState(PaladinState::GuardIdle);
			return true;
		}

		ChangeState(PaladinState::Idle);
		return false;
	}

	return TryStartGuard(pScene);
}

bool Paladin::UpdateSkillAttack(float deltaTime, Scene* pScene)
{
	if (m_SkillAttackCooldown > 0.0f)
	{
		m_SkillAttackCooldown -= deltaTime;
	}

	if (m_HasPendingSkillDamage)
	{
		m_SkillDamageDelayTimer -= deltaTime;

		if (m_SkillDamageDelayTimer <= 0.0f)
		{
			m_HasPendingSkillDamage = false;

			if (m_AttackTarget != nullptr && !m_AttackTarget->IsDestroyed())
			{
				HealthComponent* targetHp = m_AttackTarget->GetComponent<HealthComponent>();
				if (targetHp != nullptr && targetHp->IsAlive() &&
					MathUtility::IsInRange(m_Position, m_AttackTarget->GetPosition(), m_SkillAttack.range))
				{
					DamageContext context;
					context.attacker = this;
					context.damage = m_Status->CalculateSkillDamage(m_SkillAttack.skillPowerRate);
					context.isCombo = false;
					context.isCritical = false;
					m_AttackTarget->ApplyDamage(context);
				}
			}
		}
	}

	if (m_State == PaladinState::SkillAttack)
	{
		if (m_AttackTarget != nullptr && !m_AttackTarget->IsDestroyed())
		{
			m_AllyAI->FaceTarget(deltaTime, m_AttackTarget->GetPosition());
		}

		if (m_Animation != nullptr && m_Animation->GetCurrentAnimationName() == kAnimAttack && m_Animation->IsPlaying())
		{
			return true;
		}

		ChangeState(PaladinState::Idle);
		return true;
	}

	return TryStartSkillAttack(pScene);
}

bool Paladin::TryStartSkillAttack(Scene* pScene)
{
	if (pScene == nullptr) return false;

	// クールダウンが残っている場合は発動できない
	if (m_SkillAttackCooldown > 0.0f) return false;

	// 攻撃中やガード中はスキル攻撃で上書きしない。待機中・移動中のみ発動させる。
	if (IsGuarding() ||
		m_State == PaladinState::AutoAttack ||
		m_State == PaladinState::Dead)
	{
		return false;
	}

	// AllyAIComponentが管理している攻撃対象を取得。
	GameObject* enemy = m_AllyAI->GetOrFindAttackTarget(pScene);
	if (enemy == nullptr) return false;

	HealthComponent* hp = enemy->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive()) return false;

	// 攻撃対象がスキル攻撃の射程内にいない場合は発動しない
	if (!MathUtility::IsInRange(m_Position, enemy->GetPosition(), m_SkillAttack.range))
	{
		return false;
	}

	//----------------------------------------------------
	// スキル攻撃開始

	m_AttackTarget = enemy;

	// スキル発動時はターゲットの方向を向く
	m_AllyAI->FaceTarget(1.0f / 60.0f, enemy->GetPosition());

	m_HasPendingSkillDamage = true;

	// クールダウンのリセット
	m_SkillDamageDelayTimer = m_SkillAttack.effectDelaySec;
	const float cooldownRate = (m_Status != nullptr)
		? m_Status->GetSkillCooldownRate()
		: 1.0f;

	m_SkillAttackCooldown = m_SkillAttack.cooldownSec * cooldownRate;
	ChangeState(PaladinState::SkillAttack);
	return true;
}

bool Paladin::TryStartGuard(Scene* pScene)
{
	if (pScene == nullptr) return false;
	if (m_GuardCooldown > 0.0f) return false;

	// 攻撃中はガードで上書きしない。移動中・待機中のみ発動させる。
	if (m_State == PaladinState::AutoAttack ||
		m_State == PaladinState::SkillAttack ||
		m_State == PaladinState::Dead)
	{
		return false;
	}

	GameObject* enemy = m_AllyAI->GetOrFindAttackTarget(pScene);

	if (enemy == nullptr) return false;

	if (!MathUtility::IsInRange(m_Position, enemy->GetPosition(), m_GuardSkill.range))
	{
		return false;
	}

	// ガード開始
	m_GuardTimer = (m_Status != nullptr)
		? m_Status->CalculateSkillDuration(m_GuardSkill.durationSec)
		: m_GuardSkill.durationSec;
	// ガードクールダウンのリセット
	const float cooldownRate = (m_Status != nullptr)
		? m_Status->GetSkillCooldownRate()
		: 1.0f;
	m_GuardCooldown = m_GuardSkill.cooldownSec * cooldownRate;

	m_AllyAI->FaceTarget(1.0f / 60.0f, enemy->GetPosition());
	ChangeState(PaladinState::GuardStart);

	return true;
}

DamageResult Paladin::ApplyDamage(const DamageContext& context)
{
	DamageContext guardedContext = context;

	// ガード中はダメージを軽減
	if (IsGuarding())
	{
		guardedContext.damage = static_cast<int>(
			static_cast<float>(guardedContext.damage) * kGuardDamageRate
			);
		if (guardedContext.damage <= 0)
		{
			guardedContext.damage = 1;
		}
	}
	// 基本的なダメージ処理は親クラスで処理
	DamageResult result = GameObject::ApplyDamage(guardedContext);
	if (result.hit && IsGuarding())
	{
		// ガード中に攻撃を受けた場合はガードインパクト状態に移行して専用のアニメーションを再生
		ChangeState(PaladinState::GuardImpact);
	}
	return result;
}

void Paladin::ChangeState(PaladinState newState)
{
	m_State = newState;

	const char* clipName = kAnimIdle;
	bool loop = true;
	float speed = 1.0f;

	switch (m_State)
	{
	case PaladinState::Idle:
		clipName = kAnimIdle;
		loop = true;
		break;

	case PaladinState::Run:
		clipName = kAnimRun;
		loop = true;
		break;

	case PaladinState::AutoAttack:
		clipName = kAnimSlash;
		loop = false;
		break;

	case PaladinState::GuardStart:
		clipName = kAnimBlock;
		loop = false;
		break;

	case PaladinState::GuardIdle:
		clipName = kAnimBlockIdle;
		loop = true;
		break;

	case PaladinState::GuardImpact:
		clipName = kAnimBlockImpact;
		loop = false;
		break;

	case PaladinState::SkillAttack:
		clipName = kAnimAttack;
		loop = false;
		break;

	case PaladinState::Dead:
		clipName = kAnimDeath;
		loop = false;
		break;
	}

	ChangeAnimation(m_Animation, clipName, loop, speed);
}

void Paladin::ChangeAnimation(AnimationController* anim, const char* clipName, bool loop, float speed)
{
	// ステートが変化したならアニメーションも変える。
	anim->SetLoop(loop);
	anim->SetSpeed(speed);
	anim->Play(clipName);
}

StatusData Paladin::CreateStatusData() const
{
	StatusData data;
	data.maxHp = 300;
	data.moveSpeed = 45.0f;
	data.attackPower = kAutoAttackDamage;
	data.autoAttackInterval = kAutoAttackInterval;
	return data;
}

float Paladin::GetMoveSpeed() const
{
	return (m_Status != nullptr)
		? m_Status->GetMoveSpeed()
		: kFollowMoveSpeed;
}