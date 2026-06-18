#include "GameManager.h"
#include "Input.h"
#include "TitleScene.h"
#include "TerrainScene.h"
#include "Camera.h"
#include "RenderContext.h"
#include "ScreenFade.h"

// 静的メンバ変数の定義
std::unique_ptr<Scene> GameManager::m_ActiveScene = nullptr;
std::unique_ptr<Scene> GameManager::m_NextScene = nullptr;
bool GameManager::m_IsChangeScene = false;
GameStatus GameManager::m_Status = {};

//-----------------------------------------------------------------------------
//		初期化処理
//-----------------------------------------------------------------------------
void GameManager::Init()
{
	m_ActiveScene = std::make_unique<TitleScene>();
	m_ActiveScene->Init();
}

//-----------------------------------------------------------------------------
//		終了処理
//-----------------------------------------------------------------------------
void GameManager::Term()
{
	if (m_ActiveScene != nullptr)
	{
		m_ActiveScene->Term();
		m_ActiveScene.reset();
	}
	m_NextScene.reset();
}

//-----------------------------------------------------------------------------
// 		更新処理
//-----------------------------------------------------------------------------
void GameManager::Update(float deltaTime)
{
	if (m_IsChangeScene)
	{
		if (m_ActiveScene != nullptr)
		{
			m_ActiveScene->Term();
		}

		m_ActiveScene = std::move(m_NextScene);
		m_IsChangeScene = false;

		if (m_ActiveScene != nullptr)
		{
			m_ActiveScene->Init();
		}
	}

	UpdateInput();

	if (m_ActiveScene != nullptr)
	{
		m_ActiveScene->Update(deltaTime);
	}

	ScreenFade::Instance().Update(deltaTime);
}

//-----------------------------------------------------------------------------
// 		描画処理
//-----------------------------------------------------------------------------
void GameManager::Draw(const RenderContext& context, ID3D12GraphicsCommandList* pCmdList, D3D12_CPU_DESCRIPTOR_HANDLE colorRTV)
{
	if (m_ActiveScene == nullptr || pCmdList == nullptr)
		return;

	Camera* camera = m_ActiveScene->GetCamera();

	for (int i = 0; i < static_cast<int>(eLayer::MAX); ++i)
	{
		const eLayer layer = static_cast<eLayer>(i);

		// UI系は別パスで描画するためスキップ
		if (layer == eLayer::UI || layer == eLayer::MENUUI)
			continue;

		const auto& gameObjects = m_ActiveScene->GetGameObjectsByLayer(layer);
		for (const auto& obj : gameObjects)
		{
			if (!obj || obj->IsDestroyed())
				continue;

			// カリング判定
			if (obj->IsCulled() && camera != nullptr)
			{
				const DirectX::XMFLOAT3 center = obj->GetPosition();
				const float radius = obj->GetBoundingRadius();

				// 交差または内包している場合のみ描画 (0以外)
				if (camera->CollisionViewFrus(center, radius) != 0)
				{
					obj->Draw(context);
				}
			}
			else
			{
				obj->Draw(context);
			}
		}
	}

	// UIレイヤーの描画
	static const eLayer kUiLayers[] = { eLayer::UI, eLayer::MENUUI };
	for (eLayer uiLayer : kUiLayers)
	{
		const auto& uiObjects = m_ActiveScene->GetGameObjectsByLayer(uiLayer);
		for (const auto& obj : uiObjects)
		{
			if (!obj || obj->IsDestroyed()) continue;
			obj->Draw(context);
		}
	}

	// UI：深度バッファを外してスプライト等を描画
	if (colorRTV.ptr != 0)
	{
		pCmdList->OMSetRenderTargets(1, &colorRTV, FALSE, nullptr);
		const auto& uiList = m_ActiveScene->GetGameObjectsByLayer(eLayer::UI);
		for (const auto& obj : uiList)
		{
			if (obj && !obj->IsDestroyed())
			{
				obj->Draw(context);
			}
		}

		// メニューUIはさらに手前に描画するため、UIの後に描画する
		const auto& menuUIList = m_ActiveScene->GetGameObjectsByLayer(eLayer::MENUUI);
		for (const auto& obj : menuUIList)
		{
			if (obj && !obj->IsDestroyed())
			{
				obj->Draw(context);
			}
		}
	}
	ScreenFade::Instance().Draw(context);
}

//-----------------------------------------------------------------------------
//		シーン変更
//-----------------------------------------------------------------------------
void GameManager::ChangeScene(Scene* newScene)
{
	m_IsChangeScene = true;
	// 受け取った生ポインタの所有権を unique_ptr に移譲する
	m_NextScene.reset(newScene);
}