#include "TitleScene.h"
#include "TitleObject.h"
#include "Camera.h"
#include "SkyDomeObject.h"
#include "TitleChara.h"
#include "Terrain.h"
#include "Wind.h"
#include "Meadow.h"
#include "GameManager.h"
#include "TerrainScene.h"
#include "ScreenFade.h"

void TitleScene::Init()
{
    m_TitleObject = AddGameObject<TitleObject>(eLayer::UI, "TitleObj");

    Terrain* pTerrain = AddGameObject<Terrain>(eLayer::TERRAIN, "Terrain");
    pTerrain->Init(
        L"Assets/Texture/Terrain/HeightMap.png",
        L"Assets/Texture/Terrain/FieldMap.png",
        2000.0f, 10.0f);

    TitleChara* titleChara = AddGameObject<TitleChara>(eLayer::ALLY, "TitleChara");


    Camera* pCamera = AddGameObject<Camera>(eLayer::DEFAULT, "Camera");
    if (pCamera)
    {
        // 固定視点モード
        pCamera->SetFixedViewMode(true);

        // 位置はタイトルキャラに寄せる
		XMFLOAT3 titleCamPos = titleChara->GetPosition();
        titleCamPos.z += 10.0f;
		titleCamPos.y += 6.0f;
		titleCamPos.x += 3.0f;
		pCamera->SetPosition(titleCamPos); 


		XMFLOAT3 titleCamTarget = titleChara->GetPosition();
		titleCamTarget.y += 6.0f; // タイトルキャラの頭あたりを注視点にする
        titleCamTarget.x += 3.5f;
		pCamera->SetTarget(titleCamTarget);
        SetCamera(pCamera);
    }

    // 風の追加
    AddGameObject<Wind>(eLayer::Environment, "Wind");

    // 草原の追加
    float meadowSize = 0.1f;
    Meadow* pMeadow1 = AddGameObject<Meadow>(eLayer::TERRAIN, "Meadow01");
    pMeadow1->SetModelPath(L"Assets/Model/FieldObject/Grass.bmdl");
    pMeadow1->SetScale(XMFLOAT3(meadowSize, meadowSize, meadowSize));
    pMeadow1->Init(this, pTerrain);


    // スカイドームの追加
    auto* pSkyDome = AddGameObject<SkyDomeObject>(eLayer::BACKGROUND, "SkyDome");
    if (pSkyDome)
    {
        pSkyDome->SetTexturePath(L"Assets/Texture/Sky/4kHDR");
        pSkyDome->SetRadius(10000.0f);
        pSkyDome->SetSlices(32);
        pSkyDome->SetStacks(16);
    }

    m_CurrentTime = kStartTime;
}

void TitleScene::Term()
{
}

void TitleScene::Update(float deltaTime)
{
    // カウントダウン
    m_CurrentTime = std::max(0.0f, m_CurrentTime - deltaTime);


    if (IsKeyPress(VK_UP) || IsKeyPress('W'))
    {
        m_CurrentSelectIndex = 0;
    }

    if (IsKeyPress(VK_DOWN) || IsKeyPress('S'))
    {
        m_CurrentSelectIndex = 1;
    }

    m_TitleObject->SetSelectIndex(m_CurrentSelectIndex);

    if (IsKeyPress(VK_SPACE) && m_CurrentTime == 0.0f && !g_isDebugMode)
    {
        if (m_CurrentSelectIndex == 0) // スタートが選択されている場合
        {
            GameManager::ResetGameStatus(); // ゲームステータスをリセット
            GameManager::ChangeScene(new TerrainScene());
            ScreenFade::Instance().FadeIn(90, nullptr); // 30フレームかけてフェードアウト
        }
        else if (m_CurrentSelectIndex == 1) // ゲーム終了が選択されている場合
        {
            exit(0); // ゲームを終了
        }
    }

	Scene::Update(deltaTime);
}
