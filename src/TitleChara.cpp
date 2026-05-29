#include "TitleChara.h"
#include "MeshRenderer.h"
#include "AnimationController.h"
#include "GameManager.h"
#include "Scene.h"
#include "Terrain.h"
TitleChara::TitleChara()
{
	m_ModelPath = L"../Assets/Characters/MariaWProp.bmdl";
	SetPosition({ 0.0f, 0.0f, 0.0f });
	SetScale({ 0.05f, 0.05f, 0.05f });
}

bool TitleChara::Init()
{
	GameObject::Init();

	AddComponent<MeshRenderer>()->Load(m_ModelPath, m_PipelineName);
	m_AnimController = AddComponent<AnimationController>();
	m_AnimController->SetLoop(true);
	m_AnimController->Play(kAnimIdle);

	auto scene = GameManager::GetScene();
	m_Position.y = scene->GetGameObjectByName<Terrain>("Terrain")->GetHeightAt(m_Position.x, m_Position.z);

	return true;
}
