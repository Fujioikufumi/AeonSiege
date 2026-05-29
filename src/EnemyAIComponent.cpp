#include "EnemyAIComponent.h"
#include "GameObject.h"
#include "GameManager.h"
#include "Scene.h"
#include "Player.h"
#include "Paladin.h"
#include "HealthComponent.h"
#include "MathUtility.h"
#include "GameFlowUtil.h"
#include <cstdlib>

using namespace DirectX;

namespace {
	// 指定した中心点と半径の範囲内のランダムな座標を作成する
	XMFLOAT3 GetRandomPointInRadius(const XMFLOAT3& center, float radius)
	{
		float angle = (rand() % 360) * (XM_PI / 180.0f);
		float distance = (rand() % 100 / 100.0f) * radius;

		XMFLOAT3 result = center;
		result.x += cosf(angle) * distance;
		result.z += sinf(angle) * distance;
		return result;
	}
}

EnemyAIComponent::EnemyAIComponent(GameObject* pObj)
	: Component(pObj) 
{
	m_ComponentName = "EnemyAIComponent";
}

bool EnemyAIComponent::Init()
{
	// 初期ステートは、常に敵対状態ならChase、そうでなければIdle
	m_CurrentState = m_AlwaysAggro ? EnemyAIState::Chase : EnemyAIState::Idle;

	m_StateTimer = kIdleWaitTime;
	if (m_GameObject)
	{
		m_SpawnPosition = m_GameObject->GetPosition();
	}
	if (m_AnimCallback)
	{
		m_AnimCallback(m_CurrentState);
	}
	return true;
}

void EnemyAIComponent::Update(float deltaTime)
{
	if (GameFlowUtil::IsShopOpen()) return;

	if (!m_IsAIActive) return;

	if (m_GameObject == nullptr || m_GameObject->IsDestroyed()) return;

	HealthComponent* pHealth = m_GameObject->GetComponent<HealthComponent>();
	// 死亡判定（死亡したらステートを Dead にして即リターン）
	if (pHealth && !pHealth->IsAlive())
	{
		if (m_CurrentState != EnemyAIState::Dead)
		{
			GameManager::GetStatus().killCount++; // キルカウントを増加
			ChangeState(EnemyAIState::Dead);
		}
		return;
	}

	if (m_CurrentState == EnemyAIState::Dead) return;

	// -------------------------------------------------
	// クールダウンの更新
	// -------------------------------------------------
	m_MeleeCooldown = max(0.0f, m_MeleeCooldown - deltaTime);

	// -------------------------------------------------
	// ターゲットの検索・視認判定
	// -------------------------------------------------
	Scene* pScene = GameManager::GetScene();

	// 見つけたターゲットはメンバ変数 (m_CurrentTarget) にキャッシュすることを推奨します。
	GameObject* pTarget = FindAttackTarget(pScene);
	HealthComponent* targetHp = pTarget ? pTarget->GetComponent<HealthComponent>() : nullptr;

	const bool targetIsAlive = targetHp && targetHp->IsAlive();
	bool canSeeTarget = false;

	if (targetIsAlive)
	{
		const bool isDamaged = pHealth && (pHealth->GetHP() < pHealth->GetMaxHP());
		const bool inAggroRange = MathUtility::IsInRange(m_GameObject->GetPosition(), pTarget->GetPosition(), m_AggroRadius);

		canSeeTarget = m_AlwaysAggro || isDamaged || inAggroRange;
	}

	// -------------------------------------------------
	// 状態遷移の判定
	// -------------------------------------------------
	if (m_CurrentState != EnemyAIState::Attack)
	{
		if (canSeeTarget)
		{
			if (m_CurrentState == EnemyAIState::Idle || m_CurrentState == EnemyAIState::Patrol)
			{
				ChangeState(EnemyAIState::Chase);
			}
		}
		else if (m_CurrentState == EnemyAIState::Chase)
		{
			ChangeState(EnemyAIState::Idle);
		}
	}

	// -------------------------------------------------
	// 現在のステートごとの更新処理
	// -------------------------------------------------
	switch (m_CurrentState)
	{
	case EnemyAIState::Idle:
		UpdateIdle(deltaTime);
		break;

	case EnemyAIState::Patrol:
		UpdatePatrol(deltaTime);
		break;

	case EnemyAIState::Chase:
	case EnemyAIState::Attack:
		// ターゲットを見失った（死んだか範囲外）なら Idle へ
		if (!targetIsAlive && m_CurrentState != EnemyAIState::Attack)
		{
			ChangeState(EnemyAIState::Idle);
		}
		else
		{
			UpdateChaseAndAttack(deltaTime, pTarget);
		}
		break;
	}
}

void EnemyAIComponent::UpdateChaseAndAttack(float deltaTime, GameObject* target)
{
	if (target == nullptr) return;

	const XMFLOAT3 pos = m_GameObject->GetPosition();
	const XMFLOAT3 targetPos = target->GetPosition();

	const float dx = targetPos.x - pos.x;
	const float dz = targetPos.z - pos.z;
	const bool isInMelee = MathUtility::IsInRange(pos, targetPos, m_MeleeRange);

	// 回転処理 (ターゲットを睨む)
	const float targetYaw = atan2f(dx, dz);
	XMFLOAT3 rot = m_GameObject->GetRotation();
	rot.y = MathUtility::SlerpYaw(rot.y, targetYaw, 10.0f, deltaTime);
	m_GameObject->SetRotation(rot);

	if (m_CurrentState == EnemyAIState::Attack)
	{
		m_StateTimer -= deltaTime;

		// 攻撃アニメーションの持続時間が完全に終了したか？
		if (m_StateTimer <= 0.0f)
		{
			// まだ射程内にいて、かつターゲットが生きているな、そのまま攻撃状態をキープさせる
			// クールダウンが残っていても、攻撃射程内であれば Chase に遷移せずクールダウンを待つ
			if (isInMelee)
			{
				if (m_MeleeCooldown <= 0.0f)
				{
					// 再び攻撃を実行
					ChangeState(EnemyAIState::Attack);
					m_MeleeCooldown = m_MeleeCooldownSec;

					m_StateTimer = kIdleWaitTime;

					if (m_AttackCallback)
					{
						m_AttackCallback(target);
					}
				}
				else
				{
					// クールダウン中は Chase に遷移せず、攻撃射程内であればそのまま待機
				}
			}
			else
			{
				// ターゲットが射程外に逃げたので、追いかける
				ChangeState(EnemyAIState::Chase);
			}
		}
	}
	else if (m_CurrentState == EnemyAIState::Chase)
	{
		// 追跡中
		if (isInMelee && m_MeleeCooldown <= 0.0f)
		{
			ChangeState(EnemyAIState::Attack);
			m_MeleeCooldown = m_MeleeCooldownSec;

			// 攻撃のアニメーション時間をセット（これが終わるまでロックされる）
			m_StateTimer = kIdleWaitTime;

			if (m_AttackCallback)
			{
				m_AttackCallback(target);
			}
		}
		else if (!isInMelee)
		{
			// 射程外なら近づく
			const float dist = MathUtility::CalculateDistanceXZ(pos, targetPos);
			if (dist > 0.1f)
			{
				const float dirX = dx / dist;
				const float dirZ = dz / dist;

				XMFLOAT3 nextPos = pos;
				nextPos.x += dirX * m_ChaseMoveSpeed * deltaTime;
				nextPos.z += dirZ * m_ChaseMoveSpeed * deltaTime;
				m_GameObject->SetPosition(nextPos);
			}
		}
	}
}

void EnemyAIComponent::UpdateIdle(float deltaTime)
{
	if (!m_UsePatrol)
	{
		ChangeState(EnemyAIState::Chase);
		return;
	}
	m_StateTimer -= deltaTime;
	if (m_StateTimer <= 0.0f)
	{
		m_TargetPatrolPos = GetRandomPointInRadius(m_SpawnPosition, m_PatrolRadius);
		ChangeState(EnemyAIState::Patrol);
	}
}

void EnemyAIComponent::UpdatePatrol(float deltaTime)
{
	XMFLOAT3 pos = m_GameObject->GetPosition();

	float dist = MathUtility::CalculateDistanceXZ(m_GameObject->GetPosition(), m_TargetPatrolPos);
	
	// 目的地に十分近づいたら再び待機
	if (dist < 1.0f)
	{
		m_StateTimer = kIdleWaitTime + (rand() % 3); // 2～4秒待機
		ChangeState(EnemyAIState::Idle);
		return;
	}

	float dx = m_TargetPatrolPos.x - pos.x;
	float dz = m_TargetPatrolPos.z - pos.z;

	// 目的地を向く

	float targetYaw = atan2f(dx, dz);
	XMFLOAT3 rot = m_GameObject->GetRotation();
	rot.y = MathUtility::SlerpYaw(rot.y, targetYaw, 10.0f, deltaTime);
	m_GameObject->SetRotation(rot);

	// 移動
	float dirX = dx / dist;
	float dirZ = dz / dist;
	pos.x += dirX * m_MoveSpeed * deltaTime;
	pos.z += dirZ * m_MoveSpeed * deltaTime;
	m_GameObject->SetPosition(pos);
}

GameObject* EnemyAIComponent::FindAttackTarget(Scene* pScene) const
{
	if (pScene == nullptr || m_GameObject == nullptr) return nullptr;
	
	GameObject* bestTarget = nullptr;
	
	// ターゲット切り替えのスコア
	float bestScore = FLT_MAX;

	// ターゲットの候補
	auto checkTarget = [&](GameObject* target, float scoreBias)
		{
			if (target == nullptr || target->IsDestroyed())
			{
				return;
			}
			HealthComponent* hp = target->GetComponent<HealthComponent>();
			if (hp == nullptr || !hp->IsAlive())
			{
				return;
			}
			// 距離が近いほどスコアが低いようにする(優先的に狙う)
			const XMFLOAT3 selfPos = m_GameObject->GetPosition();
			const XMFLOAT3 targetPos = target->GetPosition();
			const float dx = targetPos.x - selfPos.x;
			const float dz = targetPos.z - selfPos.z;
			const float distSq = dx * dx + dz * dz;
			const float score = distSq + scoreBias;
			if (score < bestScore)
			{
				bestScore = score;
				bestTarget = target;
			}
		};

	checkTarget(pScene->GetGameObjectByName<Player>("Player"), 0.0f);
	const auto& allyList = pScene->GetGameObjectsByLayer(eLayer::ALLY);

	// 味方がガード中なら優先的に狙う
	for (const auto& allyPtr : allyList)
	{
		GameObject* ally = allyPtr.get();
		Paladin* paladin = dynamic_cast<Paladin*>(ally);
		float bias = 0.0f;
		if (paladin != nullptr && paladin->IsGuarding())
		{
			bias = -3000.0f;
		}
		checkTarget(ally, bias);
	}
	return bestTarget;
}

void EnemyAIComponent::ChangeState(EnemyAIState newState)
{
	m_CurrentState = newState;

	// もしコールバックが登録されていれば、状態が切り替わったことを親(Mutant)に知らせる
	if (m_AnimCallback)
	{
		m_AnimCallback(m_CurrentState);
	}
}
