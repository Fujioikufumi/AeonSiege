#include "Character/Zombie.h"
#include "Graphics/Renderer/MeshRenderer.h"
#include "World/Terrain.h"
#include "Core/Scene.h"
#include "Core/GameManager.h"
#include "Combat/HealthComponent.h"
#include "Graphics/Animation/AnimationController.h"
#include "Component/TargetComponent.h"
#include "AI/EnemyAIComponent.h"
#include "Combat/StatusComponent.h"
#include "Utility/MathUtility.h"
#include <random>

namespace {
	constexpr float kTargetHeightOffset = 6.0f;  // ターゲットHUDの高さオフセット
	constexpr float kAutoAttackInterval = 2.0f;  // 通常攻撃の間隔（秒）
	constexpr float kMoveSpeed = 20.0f;			 // 移動速度
	constexpr float kChaseMoveSpeed = 35.0f;	 // 追跡時の移動速度
	constexpr float kMeleeCooldownSec = 2.0f;	 // 近接攻撃のクールダウン（秒）
}

Zombie::Zombie()
	: Zombie(ZombieType::Normal)
{
}

Zombie::Zombie(ZombieType type)
	: m_Type(type)
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
	target->SetHeightOffset(kTargetHeightOffset);

	m_Status = AddComponent<StatusComponent>();
	StatusData data;
	data.maxHp = 100;
	data.attackPower = 10;
	data.autoAttackInterval = kAutoAttackInterval;
	data.criticalDamageRate = 1.5f;
	data.criticalRate = 0.3f;
	data.damageTakenRate = 1.0f;
	data.defensePower = 5;
	data.evasionRate = 0.1f;
	data.moveSpeed = kMoveSpeed;
	m_Status->Setup(data);

	HealthComponent* health = AddComponent<HealthComponent>();
	health->SetMaxHP(m_Status->GetMaxHp());
	health->SetHP(m_Status->GetMaxHp());

	AnimationController* anim = AddComponent<AnimationController>();
	anim->SetLoop(true);
	anim->Play(kAnimRun);

	m_MeleeHitRange = kMeleeHitRange; // CharacterBase の判定距離を設定

	EnemyAIComponent* enemyAi = AddComponent<EnemyAIComponent>();
	enemyAi->SetLevel(1);
	enemyAi->SetUsePatrol(false);
	enemyAi->SetAlwaysAggro(true);
	enemyAi->SetMeleeRange(kMeleeHitRange);
	enemyAi->SetChaseMoveSpeed(kChaseMoveSpeed);
	enemyAi->SetMeleeCooldownSec(kMeleeCooldownSec);

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
	if (target == nullptr)
	{
		return;
	}
	SchedulePendingMeleeDamage(target, GetAttackDamage(), kMeleeDamageDelaySec);
}