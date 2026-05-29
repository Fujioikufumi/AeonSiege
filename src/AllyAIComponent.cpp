#include "AllyAIComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "Player.h"
#include "Camera.h"
#include "Terrain.h"
#include "MathUtility.h"
#include "Component.h"
#include "HealthComponent.h"
#include <algorithm>
#include <cmath>

#include "StatusComponent.h"
using namespace DirectX;

AllyAIComponent::AllyAIComponent(GameObject* pObj)
	: Component(pObj)
{
	m_ComponentName = "AllyAIComponent";
}

bool AllyAIComponent::Init()
{
	return true;
}

void AllyAIComponent::Update(float deltaTime)
{
}

void AllyAIComponent::Setup(const AllyAIParams& params)
{
	m_Params = params;
}

bool AllyAIComponent::UpdateFollowPlayer(float deltaTime, Scene* pScene, float moveSpeed)
{
	if (m_GameObject == nullptr || pScene == nullptr) return false;
	Player* pPlayer = pScene->GetGameObjectByName<Player>("Player");
	if (pPlayer == nullptr)
	{
		m_HasWaitTarget = false;
		return false;
	}
	Camera* pCamera = pScene->GetGameObjectByName<Camera>("Camera");
	const XMFLOAT3 playerPos = pPlayer->GetPosition();
	const bool isPlayerMoving = IsPlayerMoving(playerPos);
	const float distanceToPlayer = MathUtility::CalculateDistanceXZ(m_GameObject->GetPosition(), playerPos);
	if (isPlayerMoving)
	{
		// プレイヤーが移動したら
		m_HasWaitTarget = false;
		m_IsFollowing = true;
		return MoveToTargetPosition(deltaTime, playerPos, m_Params.followStopDistance, m_Params.moveSpeed);
	}
	if (!m_IsFollowing && distanceToPlayer > m_Params.followStartDistance)
	{
		// プレイヤーから離れすぎていたら追従開始
		m_IsFollowing = true;
	}
	if (m_IsFollowing && distanceToPlayer <= m_Params.followStopDistance)
	{
		// プレイヤーに近づきすぎたら追従停止
		m_IsFollowing = false;
	}
	if (m_IsFollowing)
	{
		// プレイヤーに追従
		m_HasWaitTarget = false;
		return MoveToTargetPosition(deltaTime, playerPos, m_Params.followStopDistance, m_Params.moveSpeed);
	}
	if (!m_HasWaitTarget)
	{
		// 待機目標位置を計算して保存
		m_WaitTargetPos = CreateWaitTargetPosition(pScene, pPlayer, pCamera);
		m_HasWaitTarget = true;
	}
	return MoveToTargetPosition(deltaTime, m_WaitTargetPos, m_Params.waitTargetArriveDistance, m_Params.moveSpeed);
}

bool AllyAIComponent::MoveToTargetPosition(float deltaTime, const XMFLOAT3& targetPos, float arriveDistance, float moveSpeed)
{
	if (m_GameObject == nullptr) return false;

	// 距離を計算
	XMFLOAT3 pos = m_GameObject->GetPosition();
	const float dx = targetPos.x - pos.x;
	const float dz = targetPos.z - pos.z;
	const float distance = MathUtility::CalculateDistanceXZ(pos, targetPos);

	const float moveDistance = std::min(
		distance - arriveDistance,
		moveSpeed * deltaTime
	);

	if (distance <= arriveDistance || distance <= 0.001f)
	{
		return false;
	}
	if (moveDistance <= 0.01f)
	{
		return false;
	}
	FaceTarget(deltaTime, targetPos);
	pos.x += (dx / distance) * moveDistance;
	pos.z += (dz / distance) * moveDistance;
	m_GameObject->SetPosition(pos);
	return true;
}

void AllyAIComponent::FaceTarget(float deltaTime, const XMFLOAT3& targetPos)
{
	if (m_GameObject == nullptr) return;

	const XMFLOAT3 pos = m_GameObject->GetPosition();

	const float dx = targetPos.x - pos.x;
	const float dz = targetPos.z - pos.z;

	if ((dx * dx + dz * dz) <= 0.001f)
	{
		return;
	}

	const float targetYaw = atan2f(dx, dz);
	XMFLOAT3 rot = m_GameObject->GetRotation();
	rot.y = MathUtility::SlerpYaw(rot.y, targetYaw, m_Params.turnSpeed, deltaTime);
	m_GameObject->SetRotation(rot);
}

bool AllyAIComponent::IsPlayerMoving(const XMFLOAT3& playerPos)
{
	if (!m_HasLastPlayerPos)
	{
		m_LastPlayerPos = playerPos;
		m_HasLastPlayerPos = true;
		return false;
	}

	const float distance = MathUtility::CalculateDistanceXZ(m_LastPlayerPos, playerPos);
	m_LastPlayerPos = playerPos;

	return distance > m_Params.playerMoveThreshold;
}

XMFLOAT3 AllyAIComponent::CreateWaitTargetPosition(Scene* pScene, Player* pPlayer, Camera* pCamera)
{
	XMFLOAT3 playerPos = pPlayer->GetPosition();

	if (pCamera == nullptr)
	{
		return playerPos;
	}

	const XMFLOAT3 forward = pCamera->GetViewForwardXZ();
	const XMFLOAT3 right = pCamera->GetViewRightXZ();

	XMFLOAT3 candidates[3];

	candidates[0] = {
		playerPos.x + forward.x * m_Params.waitTargetForwardDistance + right.x * m_Params.waitTargetSideOffset,
		playerPos.y,
		playerPos.z + forward.z * m_Params.waitTargetForwardDistance + right.z * m_Params.waitTargetSideOffset
	};

	candidates[1] = {
		playerPos.x + forward.x * m_Params.waitTargetForwardDistance - right.x * m_Params.waitTargetSideOffset,
		playerPos.y,
		playerPos.z + forward.z * m_Params.waitTargetForwardDistance - right.z * m_Params.waitTargetSideOffset
	};

	candidates[2] = {
		playerPos.x + right.x * m_Params.waitTargetSideOffset,
		playerPos.y,
		playerPos.z + right.z * m_Params.waitTargetSideOffset
	};

	Terrain* pTerrain = pScene->GetGameObjectByName<Terrain>("Terrain");

	for (XMFLOAT3& candidate : candidates)
	{
		if (pTerrain != nullptr)
		{
			candidate.y = pTerrain->GetHeightAt(candidate.x, candidate.z);
		}

		if (IsValidWaitTarget(pCamera, candidate, playerPos))
		{
			return candidate;
		}
	}

	XMFLOAT3 fallback = candidates[2];
	if (pTerrain != nullptr)
	{
		fallback.y = pTerrain->GetHeightAt(fallback.x, fallback.z);
	}

	return fallback;
}

bool AllyAIComponent::IsValidWaitTarget(Camera* pCamera, const XMFLOAT3& targetPos, const XMFLOAT3& playerPos) const
{
	if (pCamera == nullptr) return false;

	const float distanceFromPlayer = MathUtility::CalculateDistanceXZ(targetPos, playerPos);
	if (distanceFromPlayer > m_Params.maxWaitTargetDistanceFromPlayer)
	{
		return false;
	}

	return pCamera->CollisionViewFrus(targetPos, 2.0f) != 0;
}

GameObject* AllyAIComponent::GetOrFindAttackTarget(Scene* pScene)
{
	// 現在のターゲットが本当に「生きている」かつ「破棄されていない」かチェック
	if (!IsValidAttackTarget(m_AttackTarget))
	{
		m_AttackTarget = nullptr;
	}

	// ターゲットがまだ有効であれば、距離チェックのみ行う
	if (m_AttackTarget != nullptr)
	{
		// ここで念のためシーンに含まれているかも追加でチェック（ContainsGameObjectは重いためポインタの生存確認を優先）
		if (pScene != nullptr && pScene->ContainsGameObject(m_AttackTarget))
		{
			if (MathUtility::IsInRange(
				m_GameObject->GetPosition(),
				m_AttackTarget->GetPosition(),
				m_Params.targetKeepRange))
			{
				return m_AttackTarget;
			}
		}

		// 範囲外 or シーンにいない場合は一旦解除
		m_AttackTarget = nullptr;
	}

	// ターゲットがいない、または無効になったので新しく探す
	m_AttackTarget = FindNearestEnemy(pScene);
	return m_AttackTarget;
}

GameObject* AllyAIComponent::FindNearestEnemy(Scene* pScene) const
{
	if (pScene == nullptr || m_GameObject == nullptr) return nullptr;

	// プレイヤーを中心に敵を索敵し、最も近い敵を返す
	GameObject* nearestEnemy = nullptr;
	float nearestDistSq = m_Params.enemyDetectRange * m_Params.enemyDetectRange;

	const auto& enemyList = pScene->GetGameObjectsByLayer(eLayer::ENEMY);

	for (const auto& enemyPtr : enemyList)
	{
		GameObject* enemy = enemyPtr.get();
		if (!IsValidAttackTarget(enemy))
		{
			continue;
		}

		const XMFLOAT3 selfPos = m_GameObject->GetPosition();
		const XMFLOAT3 enemyPos = enemy->GetPosition();

		const float dx = enemyPos.x - selfPos.x;
		const float dz = enemyPos.z - selfPos.z;
		const float distSq = dx * dx + dz * dz;

		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearestEnemy = enemy;
		}
	}

	return nearestEnemy;
}

bool AllyAIComponent::IsValidAttackTarget(GameObject* target) const
{
	if (target == nullptr || target->IsDestroyed())
	{
		return false;
	}

	HealthComponent* hp = target->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive())
	{
		return false;
	}

	return true;
}