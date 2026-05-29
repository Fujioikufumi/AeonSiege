#include "BattleArea.h"
#include "GameManager.h"
#include "Scene.h"
#include "Terrain.h"
#include <cmath>

using namespace DirectX;

BattleArea::BattleArea()
	: GameObject()
{
}

bool BattleArea::Init()
{
	GameObject::Init();
	return true;
}

void BattleArea::Setup(const XMFLOAT3& center, float radius)
{
	m_Center = center;
	m_Radius = radius;
}

void BattleArea::Update(float deltaTime)
{
	ClampLayerObjects(eLayer::PLAYER);
	ClampLayerObjects(eLayer::ALLY);
	ClampLayerObjects(eLayer::ENEMY);

	GameObject::Update(deltaTime);
}

void BattleArea::ClampLayerObjects(eLayer layer) const
{
	Scene* pScene = GameManager::GetScene();
	if (pScene == nullptr) return;

	const auto& objects = pScene->GetGameObjectsByLayer(layer);

	for (const auto& objectPtr : objects)
	{
		ClampObjectToArea(objectPtr.get());
	}
}

void BattleArea::ClampObjectToArea(GameObject* object) const
{
	if (object == nullptr || object->IsDestroyed()) return;

	XMFLOAT3 pos = object->GetPosition();

	const float dx = pos.x - m_Center.x;
	const float dz = pos.z - m_Center.z;
	const float distanceSq = dx * dx + dz * dz;
	const float radiusSq = m_Radius * m_Radius;

	if (distanceSq <= radiusSq)
	{
		return;
	}

	const float distance = sqrtf(distanceSq);
	if (distance <= 0.001f)
	{
		return;
	}

	pos.x = m_Center.x + (dx / distance) * m_Radius;
	pos.z = m_Center.z + (dz / distance) * m_Radius;

	Scene* pScene = GameManager::GetScene();
	Terrain* pTerrain = (pScene != nullptr) ? pScene->GetGameObjectByName<Terrain>("Terrain") : nullptr;
	if (pTerrain != nullptr)
	{
		pos.y = pTerrain->GetHeightAt(pos.x, pos.z);
	}

	object->SetPosition(pos);
}