#include "TitleObject.h"
#include "Input.h"
#include "Scene.h"
#include "NameSpace.h"

//----------------------------------------------------------------------
//		コンストラクタ
//----------------------------------------------------------------------
TitleObject::TitleObject()
	: GameObject()
	, m_pSprite(nullptr)
{
}

//----------------------------------------------------------------------
//		デストラクタ
//----------------------------------------------------------------------
TitleObject::~TitleObject()
{
	Term();
}

//----------------------------------------------------------------------
//		初期化
//----------------------------------------------------------------------
bool TitleObject::Init()
{
	m_pSprite = AddComponent<Sprite>();
	// スプライトコンポーネントの初期化
	if (!m_pSprite->Init(L"Assets/Texture/Title/Title.png"))
	{
		ELOG("Error : Sprite::Init() Failed");
		return false;
	}

	m_pSprite->SetPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT/ 2 + 100.0f);
	m_pSprite->SetScale(6.0f, 4.0f);
	std::wstring selectModelPath[] = {
		L"Assets/Texture/Title/Start.png",
		L"Assets/Texture/Title/QuitGame.png"
	};

	XMFLOAT2 selectModelPos[] = {
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2  + 200.0f},
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 300.0f }
	};


	for(int i = 0; i < kSelectModeCount; ++i)
	{
		m_SelectMode[i] = AddComponent<Sprite>();
		if (!m_SelectMode[i]->Init(selectModelPath[i]))
		{
			ELOG("Error : Sprite::Init() Failed");
			return false;
		}
		m_SelectMode[i]->SetPosition(selectModelPos[i]);
		m_SelectMode[i]->SetScale(1.2f, 1.0f);
	}
	return true;
}

//----------------------------------------------------------------------
//		終了処理
//----------------------------------------------------------------------
void TitleObject::Term()
{
	GameObject::Term();
}
void TitleObject::Update(float deltaTime)
{
	if (m_CurrentSelectIndex == 0)
	{
		m_SelectMode[0]->SetScale(1.4f, 1.2f); // スタートが選択されている場合は拡大
		m_SelectMode[1]->SetScale(1.0f, 1.0f); // ゲーム終了が選択されていない場合は縮小
	}
	if (m_CurrentSelectIndex == 1)
	{
		m_SelectMode[1]->SetScale(1.4f, 1.2f); // ゲーム終了が選択されている場合は拡大
		m_SelectMode[0]->SetScale(1.0f, 1.0f); // スタートが選択されていない場合は縮小
	}

	GameObject::Update(deltaTime);
}
