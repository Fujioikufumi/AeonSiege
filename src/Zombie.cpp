#include "Zombie.h"
#include "MeshRenderer.h"
#include "Terrain.h"
#include "Scene.h"
#include "GameManager.h"
#include "HealthComponent.h"
#include "AnimationController.h"
#include "TargetComponent.h"
#include "EnemyAIComponent.h"
#include "StatusComponent.h"
#include "MathUtility.h"
#include <random>

Zombie::Zombie()
	: Zombie(ZombieType::Normal)
{
}

Zombie::Zombie(ZombieType type)
	: GameObject()
	, m_Type(type)
{
	m_ModelPath = L"../Assets/Characters/Zombie.bmdl";
	m_Scale = { 0.05f, 0.05f, 0.05f };
}

Zombie::~Zombie()
{
	GameObject::Term();
}

bool Zombie::Init()
{
	if (!GameObject::Init()) return false;

	AddComponent<MeshRenderer>()->Load(m_ModelPath, m_PipelineName);

	TargetComponent* target = AddComponent<TargetComponent>();
	target->SetHeightOffset(6.0f);

	m_Status = AddComponent<StatusComponent>();
	StatusData data;
	data.maxHp = 100;
	data.attackPower = 10;
	data.autoAttackInterval = 2.0f;
	data.criticalDamageRate = 1.5f;
	data.criticalRate = 0.3f;
	data.damageTakenRate = 1.0f;
	data.defensePower = 5;
	data.evasionRate = 0.1f;
	data.moveSpeed = 20.0f;
	m_Status->Setup(data);

	HealthComponent* health = AddComponent<HealthComponent>();
	health->SetMaxHP(m_Status->GetMaxHp());
	health->SetHP(m_Status->GetMaxHp());

	AnimationController* anim = AddComponent<AnimationController>();
	anim->SetLoop(true);
	anim->Play(kAnimRun);

	EnemyAIComponent* enemyAi = AddComponent<EnemyAIComponent>();
	enemyAi->SetLevel(1);
	enemyAi->SetUsePatrol(false);
	enemyAi->SetAlwaysAggro(true);
	enemyAi->SetMeleeRange(kMeleeHitRange); // 定数 kMeleeHitRange (22.0f) を使用してDRY原則を守る
	enemyAi->SetChaseMoveSpeed(35.0f);
	enemyAi->SetMeleeCooldownSec(2.0f);

	// コールバック
	enemyAi->SetAnimCallback([this](EnemyAIState newState) { OnAIStateChanged(newState); });
	enemyAi->SetAttackCallback([this](GameObject* targetObj) { OnAIAttack(targetObj); });

	Scene* pScene = GameManager::GetScene();
	if (pScene)
	{
		Terrain* pTerrain = pScene->GetGameObject<Terrain>();
		if (pTerrain)
		{
			m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);
		}
		enemyAi->SetSpawnPosition(m_Position);
	}

	return true;
}

void Zombie::Term()
{
	GameObject::Term();
}

void Zombie::Update(float deltaTime)
{
	Scene* pScene = GameManager::GetScene();
	if (pScene == nullptr) return;

	Terrain* pTerrain = pScene->GetGameObject<Terrain>();
	if (pTerrain != nullptr)
	{
		m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);
	}

	HealthComponent* pHealth = GetComponent<HealthComponent>();
	AnimationController* pAnim = GetComponent<AnimationController>();

	// 死亡判定
	if (pHealth == nullptr || !pHealth->IsAlive())
	{
		CancelPendingMeleeDamage();

		// 死亡アニメーションが最後まで再生されたらシーンから消去
		if (pAnim != nullptr && pAnim->GetCurrentAnimationName() == kAnimDying && !pAnim->IsPlaying())
		{
			pScene->RemoveGameObject(this);
			return; // 削除されたのでUpdateは呼ばない
		}

		GameObject::Update(deltaTime);
		return;
	}

	// 生存している場合の攻撃更新
	UpdatePendingMeleeDamage(deltaTime);

	GameObject::Update(deltaTime);
}

void Zombie::SelectAttackType()
{
	m_CurrentAttackType = ZombieAttackType::Punch;

	// 通常のゾンビはパンチのみ。スケルトンゾンビはキックも織り交ぜる
	if (m_Type != ZombieType::SkeletonZombie)
	{
		return;
	}

	// 35%の確率でキック攻撃に切り替える
	static std::mt19937 engine(std::random_device{}()); // C++ の乱数生成を使用
	std::uniform_int_distribution<int> dist(0, 99);

	if (dist(engine) < kKickAttackRate)
	{
		m_CurrentAttackType = ZombieAttackType::Kick;
	}
}

int Zombie::GetAttackDamage() const
{
	switch (m_CurrentAttackType)
	{
	case ZombieAttackType::Kick:
		return static_cast<int>(m_Status->GetAttackPower() * kKickDamageRate);
	case ZombieAttackType::Punch:
	default:
		return m_Status->GetAttackPower();
	}
}

void Zombie::OnAIStateChanged(EnemyAIState newState)
{
	AnimationController* anim = GetComponent<AnimationController>();

	const char* clipName = kAnimRun;
	bool loop = true;
	float speed = 1.0f;

	switch (newState)
	{
	case EnemyAIState::Idle:
	case EnemyAIState::Patrol:
	case EnemyAIState::Chase:
		clipName = kAnimRun;
		loop = true;
		break;

	case EnemyAIState::Attack:
		SelectAttackType();
		clipName = (m_CurrentAttackType == ZombieAttackType::Kick)
			? kAnimKick
			: kAnimAttack;
		loop = false;  // 攻撃アニメーションはループさせない
		break;

	case EnemyAIState::Dead:
		clipName = kAnimDying;
		loop = false;  // 死亡アニメーションはループさせない
		break;
	}

	ChangeAnimation(anim, clipName, loop, speed);
}

void Zombie::OnAIAttack(GameObject* target)
{
	// EnemyAIComponent から攻撃イベントが呼ばれたときに、攻撃の種類を決定して、ダメージの遅延処理をセットアップ
	if (target == nullptr)
	{
		return;
	}
	m_HasPendingMeleeDamage = true;
	m_MeleeDamageDelayTimer = kMeleeDamageDelaySec;
	m_PendingMeleeTarget = target;
	m_PendingMeleeDamage = GetAttackDamage();
}

void Zombie::ChangeAnimation(AnimationController* anim, const char* clipName, bool loop, float speed)
{
	anim->SetLoop(loop);
	anim->SetSpeed(speed);
	anim->Play(clipName);
}

void Zombie::UpdatePendingMeleeDamage(float deltaTime)
{
	if (!m_HasPendingMeleeDamage)
	{
		return;
	}

	// ダメージの遅延タイマーを減算
	m_MeleeDamageDelayTimer -= deltaTime;
	if (m_MeleeDamageDelayTimer > 0.0f)
	{
		return;
	}

	m_HasPendingMeleeDamage = false;

	GameObject* target = m_PendingMeleeTarget;
	const int damage = m_PendingMeleeDamage;

	m_PendingMeleeTarget = nullptr;
	m_PendingMeleeDamage = 0;

	if (target == nullptr || target->IsDestroyed())
	{
		return;
	}

	HealthComponent* hp = target->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive())
	{
		return;
	}

	if (!MathUtility::IsInRange(m_Position, target->GetPosition(), kMeleeHitRange))
	{
		return;
	}

	DamageContext ctx;
	ctx.attacker = this;
	ctx.damage = damage;

	target->ApplyDamage(ctx);
}

void Zombie::CancelPendingMeleeDamage()
{
	m_HasPendingMeleeDamage = false;
	m_MeleeDamageDelayTimer = 0.0f;
	m_PendingMeleeTarget = nullptr;
	m_PendingMeleeDamage = 0;
}