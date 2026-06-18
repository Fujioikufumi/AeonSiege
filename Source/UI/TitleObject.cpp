#include "TitleObject.h"
#include "Input.h"
#include "Scene.h"
#include "NameSpace.h"

namespace {
	constexpr float kLogoOffsetY = 100.0f; // ロゴのY位置オフセット
	constexpr float kLogoScaleX = 6.0f;   // ロゴ拡大率X
	constexpr float kLogoScaleY = 4.0f;   // ロゴ拡大率Y
	constexpr float kMenuOffsetY1 = 200.0f; // メニュー項目1のYオフセット
	constexpr float kMenuOffsetY2 = 300.0f; // メニュー項目2のYオフセット

	constexpr XMFLOAT2  ktMenuScaleNormal = { 1.0f, 1.0f }; // メニュー項目の通常時のスケール
	constexpr XMFLOAT2  kMenuScaleSelected = { 1.4f, 1.2f }; // メニュー項目の選択時のスケール
}

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

	m_pSprite->SetPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT/ 2 + kLogoOffsetY);
	m_pSprite->SetScale(kLogoScaleX, kLogoScaleY);
	std::wstring selectModelPath[] = {
		L"Assets/Texture/Title/Start.png",
		L"Assets/Texture/Title/QuitGame.png"
	};

	XMFLOAT2 selectModelPos[] = {
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2  + kMenuOffsetY1},
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + kMenuOffsetY2 }
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
		m_SelectMode[i]->SetScale(ktMenuScaleNormal);
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
		m_SelectMode[0]->SetScale(kMenuScaleSelected); // スタートが選択されている場合は拡大
		m_SelectMode[1]->SetScale(ktMenuScaleNormal); // ゲーム終了が選択されていない場合は縮小
	}
	if (m_CurrentSelectIndex == 1)
	{
		m_SelectMode[1]->SetScale(kMenuScaleSelected); // ゲーム終了が選択されている場合は拡大
		m_SelectMode[0]->SetScale(ktMenuScaleNormal); // スタートが選択されていない場合は縮小
	}

	GameObject::Update(deltaTime);
}
