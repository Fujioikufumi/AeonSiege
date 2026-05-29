#include "Mutant.h"
#include "MeshRenderer.h"
#include "Terrain.h"
#include "Scene.h"
#include "GameManager.h"
#include "HealthComponent.h"
#include "AnimationController.h"
#include "TargetComponent.h"
#include "EnemyAIComponent.h"
#include "MathUtility.h"
#include "Player.h"
#include "NameSpace.h"

Mutant::Mutant()
	: GameObject()
{
	m_ModelPath = L"../Assets/Characters/Mutant.bmdl";
	m_Scale = { 0.1f, 0.1f, 0.1f };
}

Mutant::~Mutant()
{
	GameObject::Term();
}

bool Mutant::Init()
{
	GameObject::Init();
	AddComponent<MeshRenderer>()->Load(m_ModelPath, m_PipelineName);

	TargetComponent* target = AddComponent<TargetComponent>();
	target->SetHeightOffset(10.0f);

	AnimationController* anim = AddComponent<AnimationController>();
	anim->Init(m_ModelPath);
	anim->SetLoop(true);
	anim->Play(kAnimIdle);

	// AIコンポーネント
	EnemyAIComponent* ai = AddComponent<EnemyAIComponent>();
	ai->SetLevel(1);
	ai->SetUsePatrol(false);
	ai->SetAlwaysAggro(true); // 常に敵対状態
	ai->SetMeleeRange(kMeleeRange);
	ai->SetChaseMoveSpeed(kChaseMoveSpeed);
	ai->SetMeleeCooldownSec(kMeleeCooldownSec);

	m_Status = AddComponent<StatusComponent>();
	StatusData data = {};
	{
		data.maxHp = 250;
		data.attackPower = 15;
		data.autoAttackInterval = kMeleeCooldownSec;
		data.criticalDamageRate = 1.5f;
		data.criticalRate = 0.2f;
		data.damageTakenRate = 1.0f;
		data.defensePower = 8;
		data.evasionRate = 0.05f;
		data.moveSpeed = 25.0f;
	}
	m_Status->Setup(data);

	m_pHealth = AddComponent<HealthComponent>();
	m_pHealth->SetMaxHP(m_Status->GetMaxHp());
	m_pHealth->SetHP(m_Status->GetMaxHp());

	// AIからのコールバック
	// 状態が変わったときにアニメーションを変える
	ai->SetAnimCallback([this](EnemyAIState newState) {
		this->OnAIStateChanged(newState);
		});

	// AIコンポーネントから攻撃距離に入ってクールダウンが明けたときに攻撃ロジックを実行してもらう
	ai->SetAttackCallback([this](GameObject* target) {
		this->OnAIAttack(target);
		});

	Scene* scene = GameManager::GetScene();
	if (scene)
	{
		Terrain* pTerrain = scene->GetGameObject<Terrain>();
		if (pTerrain)
			m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);

		// 湧き位置を現在の座標としてAIに設定
		ai->SetSpawnPosition(m_Position);
	}

	return true;
}

void Mutant::Term()
{
	GameObject::Term();
}

void Mutant::Update(float deltaTime)
{
	Scene* scene = GameManager::GetScene();
	if (scene == nullptr) return;
	Terrain* pTerrain = scene->GetGameObjectByName<Terrain>("Terrain");
	if (pTerrain == nullptr) return;
	const float fGroundY = pTerrain->GetHeightAt(m_Position.x, m_Position.z);

	// 死亡時は地面に接地させる
	if (!m_pHealth->IsAlive())
	{
		m_Position.y = fGroundY;
	}

	AnimationController* pAnim = GetComponent<AnimationController>();
	if (!m_pHealth->IsAlive())
	{
		CancelPendingMeleeDamage();

		// 死亡アニメーションが開始されていない場合は強制的に開始する
		// (ジャンプ中などでAIが停止しており、OnAIStateChangedが呼ばれなかった場合への対処)
		if (pAnim != nullptr && pAnim->GetCurrentAnimationName() != kAnimDying)
		{
			ChangeAnimation(pAnim, kAnimDying, false, 1.0f);
			m_JumpSkillState = MutantJumpSkillState::Disabled;
		}

		if (pAnim != nullptr && pAnim->GetCurrentAnimationName() == kAnimDying && !pAnim->IsPlaying())
		{
			scene->RemoveGameObject(this);
			return;
		}
		GameObject::Update(deltaTime);
		return;
	}

	// 実行中の場合はY座標の自律制御を行う（ジャンプ中など）
	if (m_JumpSkillState != MutantJumpSkillState::Executing)
	{
		m_Position.y = fGroundY;
	}

	UpdateJumpAttackSkill(deltaTime);
	UpdatePendingMeleeDamage(deltaTime);
	GameObject::Update(deltaTime);
}
void Mutant::OnAIStateChanged(EnemyAIState newState)
{
	// ジャンプ中は AI によるアニメ上書きを防ぐ
	if (m_JumpSkillState == MutantJumpSkillState::Executing && newState != EnemyAIState::Dead)
		return;
	AnimationController* anim = GetComponent<AnimationController>();
	if (anim == nullptr || !anim->IsInitialized()) return;
	const char* clipName = kAnimIdle;
	bool loop = true;
	float speed = 1.0f;
	switch (newState)
	{
	case EnemyAIState::Idle:
		clipName = kAnimIdle;
		break;
	case EnemyAIState::Patrol:
		clipName = kAnimWalk;
		break;
	case EnemyAIState::Chase:
		clipName = kAnimRun;
		break;
	case EnemyAIState::Attack:
		clipName = kAnimSwiping;
		loop = false;
		break;
	case EnemyAIState::Dead:
		clipName = kAnimDying;
		loop = false;
		break;
	}
	ChangeAnimation(anim, clipName, loop, speed);
}
void Mutant::OnAIAttack(GameObject* target)
{
	if (target == nullptr)
		return;
	// ジャンプ発動待ち・実行中は通常攻撃を予約しない
	if (m_JumpSkillState == MutantJumpSkillState::WaitCurrentAnim
		|| m_JumpSkillState == MutantJumpSkillState::Executing)
	{
		return;
	}
	m_HasPendingMeleeDamage = true;
	m_MeleeDamageDelayTimer = kMeleeDamageDelaySec;
	m_PendingMeleeTarget = target;
	m_PendingMeleeDamage = m_Status->GetAttackPower();
}
void Mutant::ChangeAnimation(AnimationController* anim, const char* clipName, bool loop, float speed)
{
	if (anim == nullptr || !anim->IsInitialized() || clipName == nullptr) return;
	anim->SetLoop(loop);
	anim->SetSpeed(speed);
	anim->Play(clipName);
}
void Mutant::CancelPendingMeleeDamage()
{
	m_HasPendingMeleeDamage = false;
	m_MeleeDamageDelayTimer = 0.0f;
	m_PendingMeleeTarget = nullptr;
	m_PendingMeleeDamage = 0;
}
void Mutant::UpdatePendingMeleeDamage(float deltaTime)
{
	if (!m_HasPendingMeleeDamage)
		return;
	m_MeleeDamageDelayTimer -= deltaTime;
	if (m_MeleeDamageDelayTimer > 0.0f)
		return;
	m_HasPendingMeleeDamage = false;
	GameObject* target = m_PendingMeleeTarget;
	const int damage = m_PendingMeleeDamage;
	m_PendingMeleeTarget = nullptr;
	m_PendingMeleeDamage = 0;
	if (target == nullptr || target->IsDestroyed())
		return;
	HealthComponent* hp = target->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive())
		return;
	if (!MathUtility::IsInRange(m_Position, target->GetPosition(), kMeleeHitRange))
		return;
	DamageContext context{};
	context.attacker = this;
	context.damage = damage;
	target->ApplyDamage(context);
}
//-----------------------------------------------------------------------------
// ジャンプ攻撃スキル
//-----------------------------------------------------------------------------
bool Mutant::IsEnraged() const
{
	return m_pHealth->GetHPRatio() <= kEnrageHPRatio;
}
bool Mutant::CanStartJumpAttackNow(const AnimationController* anim) const
{
	if (anim == nullptr || !anim->IsInitialized())
		return true;
	const std::string& name = anim->GetCurrentAnimationName();
	// 自動攻撃（1ショット）の最中は終了を待つ
	if (name == kAnimSwiping)
		return !anim->IsPlaying();
	if (name == kAnimJumpAttack || name == kAnimDying)
		return false;
	return true;
}
void Mutant::UpdateJumpAttackSkill(float deltaTime)
{
	const bool isEnraged = IsEnraged();
	if (!isEnraged)
	{
		m_JumpSkillState = MutantJumpSkillState::Disabled;
		m_WasEnraged = false;
		return;
	}
	// 初めて HP 50% 以下になった瞬間に CD を開始
	if (!m_WasEnraged)
	{
		m_JumpSkillState = MutantJumpSkillState::Cooldown;
		m_JumpAttackCooldownTime = kJumpAttackCooldownSec;
		m_WasEnraged = true;
	}
	switch (m_JumpSkillState)
	{
	case MutantJumpSkillState::Cooldown:
		m_JumpAttackCooldownTime -= deltaTime;
		if (m_JumpAttackCooldownTime <= 0.0f)
			m_JumpSkillState = MutantJumpSkillState::WaitCurrentAnim;
		break;
	case MutantJumpSkillState::WaitCurrentAnim:
		if (CanStartJumpAttackNow(GetComponent<AnimationController>()))
			StartJumpAttack();
		break;
	case MutantJumpSkillState::Executing:
		UpdateJumpAttackExecution(deltaTime);
		break;
	default:
		break;
	}
}
void Mutant::StartJumpAttack()
{
	CancelPendingMeleeDamage();
	if (EnemyAIComponent* ai = GetComponent<EnemyAIComponent>())
		ai->SetAIActive(false);
	ChangeAnimation(GetComponent<AnimationController>(), kAnimJumpAttack, false, 1.0f);
	m_JumpSkillState = MutantJumpSkillState::Executing;
	m_JumpLandTimer = kJumpAttackDamageDelaySec;
	m_JumpDamageApplied = false;
}
void Mutant::UpdateJumpAttackExecution(float deltaTime)
{
	AnimationController* anim = GetComponent<AnimationController>();
	// 着地ダメージ
	if (!m_JumpDamageApplied)
	{
		m_JumpLandTimer -= deltaTime;
		if (m_JumpLandTimer <= 0.0f)
		{
			ApplyJumpAttackDamageToAllAllies();
			m_JumpDamageApplied = true;
		}
		return;  // ダメージ前はアニメ終了判定しない
	}
	// ② ジャンプアニメ終了待ち
	if (anim == nullptr || !anim->IsInitialized())
		return;
	if (anim->GetCurrentAnimationName() != kAnimJumpAttack)
		return;
	if (anim->IsPlaying())
		return;
	FinishJumpAttack();
}
void Mutant::ApplyJumpAttackDamageToAllAllies()
{
	Scene* scene = GameManager::GetScene();
	if (scene == nullptr || m_Status == nullptr)
		return;
	DamageContext context{};
	context.attacker = this;
	context.damage = static_cast<int>(m_Status->GetAttackPower() * kJumpAttackDamageRate);
	auto applyTo = [&](GameObject* obj)
		{
			if (obj == nullptr || obj->IsDestroyed())
				return;
			HealthComponent* hp = obj->GetComponent<HealthComponent>();
			if (hp == nullptr || !hp->IsAlive())
				return;
			obj->ApplyDamage(context);
		};
	applyTo(scene->GetGameObjectByName<Player>("Player"));
	const auto& allyList = scene->GetGameObjectsByLayer(eLayer::ALLY);
	for (const auto& allyPtr : allyList)
		applyTo(allyPtr.get());
}

void Mutant::FinishJumpAttack()
{
	if (EnemyAIComponent* ai = GetComponent<EnemyAIComponent>())
		ai->SetAIActive(true);
	m_JumpAttackCooldownTime = kJumpAttackCooldownSec;
	m_JumpSkillState = MutantJumpSkillState::Cooldown;
	ResumeAutoAttackAnimation();
}

void Mutant::ResumeAutoAttackAnimation()
{
	AnimationController* anim = GetComponent<AnimationController>();
	if (anim == nullptr || !anim->IsInitialized())
		return;
	ChangeAnimation(anim, kAnimSwiping, false, 1.0f);
}
