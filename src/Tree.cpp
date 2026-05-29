#include "Tree.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "Terrain.h"
#include "GameManager.h"

//-----------------------------------------------------------------------------
// 		コンストラクタ
//-----------------------------------------------------------------------------
Tree::Tree()
	: GameObject()
{
}

//-----------------------------------------------------------------------------
// 		デストラクタ
//-----------------------------------------------------------------------------
Tree::~Tree()
{
}

//-----------------------------------------------------------------------------
// 		初期化処理
//-----------------------------------------------------------------------------
bool Tree::Init()
{
	GameObject::Init();
	AddComponent<MeshRenderer>()->Load(L"Assets/Model/FieldObject/Tree/Tree01.bmdl", L"ModelPipeline");
	m_Scale = { 1.0f, 1.0f, 1.0f };
	Scene* pScene = GameManager::GetScene();
	if (pScene)
	{
		Terrain* pTerrain = pScene->GetGameObject<Terrain>();
		if (pTerrain)
			m_Position.y = pTerrain->GetHeightAt(m_Position.x, m_Position.z);
	}
	return true;
}

//-----------------------------------------------------------------------------
// 		終了処理
//-----------------------------------------------------------------------------
void Tree::Term()
{
	GameObject::Term();
}

void Tree::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}