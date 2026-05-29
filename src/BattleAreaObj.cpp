#include "BattleAreaObj.h"
#include "GameManager.h"
#include "Scene.h"
#include "Terrain.h"
#include "Tree.h"
#include <cstdlib>
#include <cmath>

using namespace DirectX;

BattleAreaObj::BattleAreaObj()
	: GameObject()
{
}

bool BattleAreaObj::Init()
{
	GameObject::Init();
	return true;
}

void BattleAreaObj::Term()
{
	GameObject::Term();
}

void BattleAreaObj::Setup(const XMFLOAT3& center, float radius)
{
	m_Center = center;
	m_Radius = radius;

	if (!m_HasPlaced)
	{
		// PlaceBoundaryTrees();
		m_HasPlaced = true;
	}
}

void BattleAreaObj::PlaceBoundaryTrees()
{
	Scene* pScene = GameManager::GetScene();
	if (pScene == nullptr) return;

	Terrain* pTerrain = pScene->GetGameObjectByName<Terrain>("Terrain");
	if (pTerrain == nullptr) return;

	const float minRadius = m_Radius + kInnerRadiusOffset;
	const float maxRadius = m_Radius + kOuterRadiusOffset;
	const float angleStep = XM_2PI / static_cast<float>(kTreeCount);

	for (int i = 0; i < kTreeCount; ++i)
	{
		const float angle = angleStep * static_cast<float>(i) +
			RandomRange(-kAngleRandomRange, kAngleRandomRange);

		const float treeRadius = RandomRange(minRadius, maxRadius);

		XMFLOAT3 pos;
		pos.x = m_Center.x + cosf(angle) * treeRadius;
		pos.z = m_Center.z + sinf(angle) * treeRadius;
		pos.y = pTerrain->GetHeightAt(pos.x, pos.z);

		std::string treeName = "BoundaryTree_" + std::to_string(i);
		Tree* tree = pScene->AddGameObject<Tree>(eLayer::DEFAULT , treeName);
		if (tree == nullptr)
		{
			continue;
		}

		tree->SetPosition(pos);
		tree->SetRotation({ 0.0f, RandomRange(0.0f, XM_2PI), 0.0f });

		const float scale = RandomRange(kMinTreeScale, kMaxTreeScale);
		tree->SetScale({ scale, -scale, scale });
		tree->SetIsCulled(true);
	}
}

float BattleAreaObj::RandomRange(float minValue, float maxValue) const
{
	const float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	return minValue + (maxValue - minValue) * t;
}