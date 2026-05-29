//-----------------------------------------------------------------------------
//      Includes
//-----------------------------------------------------------------------------
#include "GameApp.h"
#include "FileUtil.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "Input.h"
#include "ResourceManager.h"
#include "GameManager.h"
#include "Scene.h"
#include "LightManager.h"
#include "ShadowManager.h"
#include "AnimationManager.h"
#include "PipelineInitializer.h"
#include "Camera.h"
#include "LoadGameObj.h"
#include "ScreenFade.h"

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
GameApp::GameApp(uint32_t width, uint32_t height)
	: App(width, height, DXGI_FORMAT_R10G10B10A2_UNORM)
	, m_RotateAngle(0.0f)
{
}

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
GameApp::~GameApp()
{
}

//-----------------------------------------------------------------------------
//      初期化処理
//-----------------------------------------------------------------------------
bool GameApp::OnInit()
{
	// 1. リソースマネージャーの初期化
	if (!ResourceManager::GetInstance().Init(m_pDevice, m_pPool, m_pQueue.Get()))
	{
		ELOG("Error : ResourceManager::Init() Failed");
		return false;
	}

	// 2. モデルマネージャーの初期化
	if (!ModelManager::GetInstance().Init())
	{
		ELOG("Error : ModelManager::Init() Failed");
		return false;
	}

	// 2-1. アニメーションマネージャーの初期化
	if (!AnimationManager::GetInstance().Init())
	{
		ELOG("Error : AnimationManager::Init() Failed");
		return false;
	}
	// 3. パイプライン初期化
	if (!PipelineInitializer::InitializeAllPipelines())
	{
		ELOG("Error : PipelineInitializer::InitializeAllPipelines() Failed");
		return false;
	}

	// 4. RenderSystem初期化
	if (!m_RenderSystem.Init())
	{
		ELOG("Error : RenderSystem::Init() Failed");
		return false;
	}
	ResourceManager::GetInstance().SetRenderSystem(&m_RenderSystem);

	//-------------------------------------------------------------
	// 5. アプリケーション固有のリソース初期化
	//-------------------------------------------------------------
	// シーン用カラ―ターゲット
	{
		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		if (!m_SceneColorTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_RTV],
			m_pPool[POOL_TYPE_RES],
			m_Width,
			m_Height,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			clearColor))
		{
			ELOG("Error : ColorTarget::Init() Failed");
			return false;
		}
	}

	// シーン用深度ターゲット
	{
		if (!m_SceneDepthTarget.Init(
			m_pDevice.Get(),
			m_pPool[POOL_TYPE_DSV],
			nullptr,
			m_Width,
			m_Height,
			DXGI_FORMAT_D32_FLOAT,
			1.0f,
			0))
		{
			ELOG("Error : DepthTarget::Init() Failed");
			return false;
		}
	}

	// 定数バッファの初期化
	{
		for (auto i = 0u; i < FrameCount; ++i)
		{
			if (!m_TransformCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTransform)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed");
				return false;
			}

			if (!m_CameraCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbCamera)))
			{
				ELOG("Error : ConstantBuffer::Init() Failed");
				return false;
			}

			if (!m_MeshCB[i].Init(m_pDevice.Get(),
				m_pPool[POOL_TYPE_RES],
				sizeof(CbMesh),
				FrameCount))
			{
				ELOG("Error : ConstantBuffer::Init() Failed");
				return false;
			}

			// 初期値を設定
			XMVECTOR eyePos = XMVectorSet(0.0f, 1.0f, 2.0f, 0.0f);
			XMVECTOR targetPos = XMVectorZero();
			XMVECTOR upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			float fovY = DirectX::XMConvertToRadians(37.5f);
			float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
			XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, targetPos, upward);
			XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(fovY, aspect, 1.0f, 1000.0f);

			auto ptr = m_TransformCB[i].GetPtr<CbTransform>();
			XMMATRIX tv = XMMatrixTranspose(viewMatrix);
			XMMATRIX tp = XMMatrixTranspose(projMatrix);
			XMStoreFloat4x4(&ptr->View, tv);
			XMStoreFloat4x4(&ptr->Proj, tp);

			auto meshPtr = m_MeshCB[i].GetPtr<CbMesh>(i);
			XMMATRIX identity = XMMatrixIdentity();
			XMStoreFloat4x4(&meshPtr->World, identity);
		}

		m_RotateAngle = DirectX::XMConvertToRadians(-60.0f);
	}

	// 6. ライトマネージャーの初期化
	if (!LightManager::GetInstance().Init())
	{
		ELOG("Error : LightManager::Init() Failed");
		return false;
	}

	// 7. シャドウマネージャーの初期化
	if (!ShadowManager::GetInstance().Init())
	{
		ELOG("Error : ShadowManager::Init() Failed");
		return false;
	}

	// アニメーションのロード
	{
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Walking.banm", "Walk");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Running.banm", "Run");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Jump.banm", "Jump");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Idle.banm", "Idle");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash.banm", "Player_Attack01");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash1.banm", "Player_Skill01");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash2.banm", "Player_Skill02");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash3.banm", "Player_Skill03");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash4.banm", "Player_Skill04");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash5.banm", "Player_Skill05");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Slash6.banm", "Player_Skill06");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Casting.banm", "Player_Casting");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Great_Sword_Idle.banm", "Player_Combat_Idle");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/PlayerAnimation/Two_Handed_Sword_Death.banm", "Player_Combat_Dying");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Walking.banm", "Mutant_Normal_Walk");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Idle.banm", "Mutant_Normal_Idle");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Run.banm", "Mutant_Combat_Run");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Swiping.banm", "Mutant_Combat_Swiping");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Jump_Attack.banm", "Mutant_Combat_JumpAttack");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Mutant_Dying.banm", "Mutant_Combat_Dying");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Zombie_Running.banm", "Zombie_Run");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Zombie_Punching.banm", "Zombie_Attack");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Zombie_Kick.banm", "Zombie_Kick");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Zombie_Death.banm", "Zombie_Dying");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Idle.banm", "Paladin_Idle");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Run.banm", "Paladin_Run");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Slash.banm", "Paladin_Slash");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Block.banm", "Paladin_Block");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Block_Idle.banm", "Paladin_Block_Idle");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Block_Impact.banm", "Paladin_Block_Impact");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Attack.banm", "Paladin_Attack");
		AnimationManager::GetInstance().LoadAnimation(L"Assets/Animations/Paladin_Death.banm", "Paladin_Death");
	}

	//　入力処理の初期化
	{
		InitInput();
		SetInputWindow(m_hWnd); // ウィンドウハンドルを設定
		LockMouse(true);        // マウスをウィンドウ中央に固定
	}

	// 8. シーンマネージャーの初期化
	{
		GameManager::Init();
		ScreenFade::Instance().Init();
	}

	// 9. ImGuiの初期化
	if(!m_ImGuiUtil.Init(m_hWnd, m_pDevice.Get(), FrameCount, m_BackBufferFormat))
	{
		ELOG("Error : ImGuiUtil::Init() Failed");
		return false;
	}

	{
		Camera* pCamera = GameManager::GetScene()->GetCamera();
		// 10. デバッグUIの初期化
		m_DebugUI.SetCamera(pCamera);
		m_DebugUI.SetScene(GameManager::GetScene());
	}
	return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void GameApp::OnTerm()
{
	// ImGuiの終了処理
	m_ImGuiUtil.Term();

	ScreenFade::Instance().Term();
	GameManager::Term();


	LockMouse(false);   // マウス固定解除
	UninitInput();

	/*SafeDelete(m_pCamera);*/
	AnimationManager::GetInstance().Term();

	// コンスタントバッファの終了処理
	for (auto i = 0; i < FrameCount; ++i)
	{
		m_TransformCB[i].Term();
		m_CameraCB[i].Term();
		m_MeshCB[i].Term();
	}

	// シーン用ターゲットの終了処理
	m_SceneColorTarget.Term();
	m_SceneDepthTarget.Term();

	// レンダリングシステムの終了処理
	m_RenderSystem.Term();
}

//-----------------------------------------------------------------------------
// 	    更新処理
//-----------------------------------------------------------------------------
void GameApp::OnUpdate()
{
	// ImGuiのフレーム開始
	m_ImGuiUtil.NewFrame();

	// デバックUIの更新
	float deltaTime = GetDeltaTime();
	m_DebugUI.Update(deltaTime, m_FrameIndex, m_Width, m_Height);
	m_DebugUI.SetScene(GameManager::GetScene()); // シーンが変更される可能性を考慮

	// ゲームオブジェクト更新
	GameManager::Update(deltaTime);
}

//-----------------------------------------------------------------------------
//      描画処理
//-----------------------------------------------------------------------------
void GameApp::OnRender()
{
	// 1. コマンドリストの取得とリセット
	auto pCmd = m_CommandList.Reset();

	// 2. ディスクリプタヒープの設定
	ID3D12DescriptorHeap* const pHeaps[] = {
		m_pPool[POOL_TYPE_RES]->GetHeap(),
	};
	pCmd->SetDescriptorHeaps(1, pHeaps);

	// 3. レンダーターゲットのリソースステート遷移と描画
	{
		// 3-1. カラーバッファをPRESENT状態からRENDER_TARGET状態に遷移
		DirectX::TransitionResource(pCmd,
			m_ColorTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// 3-2. レンダーターゲットと深度バッファのハンドルを取得
		auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
		auto handleDSV = m_DepthTarget.GetHandleDSV();

		// 3-3. レンダーターゲットと深度バッファを設定
		pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

		// 3-4. レンダーターゲットと深度バッファをクリア
		float clearColor[4] = { 0.2f, 0.2f, 0.4f, 1.0f };
		pCmd->ClearRenderTargetView(handleRTV->HandleCPU, clearColor, 0, nullptr);
		pCmd->ClearDepthStencilView(handleDSV->HandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 3-5. ビューポートとシザー矩形を設定
		pCmd->RSSetViewports(1, &m_Viewport);
		pCmd->RSSetScissorRects(1, &m_Scissor);

		// 3-6. シーンの描画（3Dオブジェクト、UIなど）
		DrawScene(pCmd);

		// ImGuiの描画
		m_ImGuiUtil.Render(pCmd);

		// 3-7. カラーバッファをRENDER_TARGET状態からPRESENT状態に遷移
		DirectX::TransitionResource(pCmd,
			m_ColorTarget[m_FrameIndex].GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
	}

	// 4. コマンドリストをクローズ
	pCmd->Close();

	// 5. コマンドリストを実行キューに登録して実行
	ID3D12CommandList* pLists[] = { pCmd };
	m_pQueue->ExecuteCommandLists(1, pLists);

	// 6. 画面に表示（スワップチェーンをフリップ）
	Present(1);
}

//-----------------------------------------------------------------------------
//      シーン描画
//-----------------------------------------------------------------------------
void GameApp::DrawScene(ID3D12GraphicsCommandList* pCmdList)
{
	// 1. ライトマネージャーの更新
	UpdateLightManager();

	// 2. シャドウマップ生成
	//RenderShadowMap(pCmdList);

	// 3. 定数バッファの更新
	UpdateConstantBuffers();

	// 4. RenderContext構築
	RenderContext context = CreateRenderContext(pCmdList);

	// 5. レンダーターゲット設定
	auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
	auto handleDSV = m_DepthTarget.GetHandleDSV();
	pCmdList->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);
	pCmdList->RSSetViewports(1, &m_Viewport);
	pCmdList->RSSetScissorRects(1, &m_Scissor);

	// 6. ゲームオブジェクトの描画
	GameManager::Draw(context, pCmdList, handleRTV->HandleCPU);
}



//-----------------------------------------------------------------------------
//	  RenderContextの作成
//-----------------------------------------------------------------------------
RenderContext GameApp::CreateRenderContext(ID3D12GraphicsCommandList* pCmdList)
{
	// 定数バッファのGPUハンドル取得
	RenderContext context;
	context.pCmdList = pCmdList;
	context.transformCB = m_TransformCB[m_FrameIndex].GetHandleGPU();
	context.lightCB = LightManager::GetInstance().GetHandleGPU(m_FrameIndex);
	context.cameraCB = m_CameraCB[m_FrameIndex].GetHandleGPU();
	context.meshCB = m_MeshCB[m_FrameIndex].GetHandleGPU(m_FrameIndex);

	auto& shadowManager = ShadowManager::GetInstance();
	context.shadowCB = shadowManager.GetHandleGPU(m_FrameIndex);
	context.shadowMapSRV = shadowManager.GetShadowMapSRV();

	// Sceneのカメラを取得
	Scene* pScene = GameManager::GetScene();
	Camera* pActiveCamera = nullptr;

	if (pScene && pScene->GetCamera())
	{
		pActiveCamera = pScene->GetCamera();
	}

	// RenderContextに渡すカメラ情報の設定
	if (pActiveCamera)
	{
		context.viewMatrix = pActiveCamera->GetView();
		context.projMatrix = pActiveCamera->GetProj();
		context.viewProjMatrix = context.viewMatrix * context.projMatrix;
		context.cameraPos = pActiveCamera->GetPosition();
	}

	context.viewport = m_Viewport;
	context.scissorRect = m_Scissor;

	return context;
}

//-----------------------------------------------------------------------------
//	  影の描画対象オブジェクトの収集
//-----------------------------------------------------------------------------
std::list<GameObject*> GameApp::CollectShadowCasters(Scene* pScene)
{
	std::list<GameObject*> shadowCasters;
	if (pScene == nullptr) return shadowCasters;

	const auto& playerList = pScene->GetGameObjectsByLayer(eLayer::PLAYER);
	for (const auto& up : playerList)
	{
		if (up)
			shadowCasters.push_back(up.get());
	}
	return shadowCasters;
}

//-----------------------------------------------------------------------------
//	  ライトマネージャーの更新
//-----------------------------------------------------------------------------
void GameApp::UpdateLightManager()
{
	// ライトマネージャーの取得
	auto& lightManager = LightManager::GetInstance();

	// ライトの設定
	const float radius = 300.0f;
	const float lightHeight = 100.0f;
	float lightX = radius * cosf(m_RotateAngle);
	float lightZ = radius * sinf(m_RotateAngle);
	float lightY = lightHeight;

	// ライト方向の計算
	XMVECTOR lightPosition = XMVectorSet(lightX, lightY, lightZ, 0.0f);
	XMVECTOR center = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR lightDirVec = XMVector3Normalize(XMVectorSubtract(center, lightPosition));

	// ライト情報の設定
	XMFLOAT3 lightColor(1.0f, 1.0f, 1.0f);
	lightManager.SetLightColor(lightColor);
	lightManager.SetLightDirection(lightDirVec);
	lightManager.SetLightIntensity(6.0f);

	XMFLOAT3 ambientColor(0.9f, 0.9f, 0.9f);
	lightManager.SetAmbientColor(ambientColor);
	lightManager.SetAmbientIntensity(0.6f);

	lightManager.Update(m_FrameIndex);
	m_RotateAngle += 0.001f;
}

//-----------------------------------------------------------------------------
//	  シャドウマップのレンダリング
//-----------------------------------------------------------------------------
void GameApp::RenderShadowMap(ID3D12GraphicsCommandList* pCmdList)
{
	RenderContext context = CreateRenderContext(pCmdList);
	auto& shadowManager = ShadowManager::GetInstance();
	auto& lightManager = LightManager::GetInstance();

	XMVECTOR lightDirVec = lightManager.GetLightDirection();
	XMFLOAT3 lightDir;
	XMStoreFloat3(&lightDir, lightDirVec);

	std::list<GameObject*> shadowCasters = CollectShadowCasters(GameManager::GetScene());
	shadowManager.RenderShadowMap(pCmdList, lightDir, shadowCasters);
	shadowManager.Update(m_FrameIndex, lightDir);
}

//-----------------------------------------------------------------------------
//	  定数バッファの更新
//-----------------------------------------------------------------------------
void GameApp::UpdateConstantBuffers()
{
	Scene* pScene = GameManager::GetScene();
	if(pScene == nullptr || pScene->GetCamera() == nullptr)
	{
		return;
	}

	Camera* pCamera = pScene->GetCamera();

	// カメラ位置の更新
	{
		auto ptr = m_CameraCB[m_FrameIndex].GetPtr<CbCamera>();
		ptr->cbCamPos = pCamera->GetPosition();
	}

	// ビュー・プロジェクション行列の更新
	{
		float fovY = pCamera->GetFovY();
		float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
		XMMATRIX viewMatrix = pCamera->GetView();
		XMMATRIX projMatrix = pCamera->GetProj();

		CbTransform* ptr = m_TransformCB[m_FrameIndex].GetPtr<CbTransform>();

		XMMATRIX tv = XMMatrixTranspose(viewMatrix);
		XMMATRIX tp = XMMatrixTranspose(projMatrix);
		XMStoreFloat4x4(&ptr->View, tv);
		XMStoreFloat4x4(&ptr->Proj, tp);
	}
}

//-----------------------------------------------------------------------------
//  	ウィンドウメッセージ処理
//-----------------------------------------------------------------------------
void GameApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	// ImGuiのウィンドウメッセージ処理
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp))
	{
		return;
	}

	// ESCAPEキーで終了
	if ((msg == WM_KEYDOWN) && (wp == VK_ESCAPE))
	{
		PostQuitMessage(0);
	}
}