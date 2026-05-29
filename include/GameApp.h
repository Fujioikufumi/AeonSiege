#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "App.h"
#include "RenderSystem.h"
#include "ShaderManager.h"
#include "PipelineStateManager.h"
#include "ModelManager.h"
#include "ConstantBuffer.h"
#include "ColorTarget.h"
#include "DepthTarget.h"
#include "Logger.h"
#include "NameSpace.h"
#include "main.h"
#include "Scene.h"
#include "ImGuiUtil.h"
#include "DebugUI.h"

//-----------------------------------------------------------------------------
//  GameApp class
//-----------------------------------------------------------------------------
class GameApp : public App
{
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    GameApp(uint32_t width, uint32_t height);

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~GameApp();

private:
    // レンダラーシステム
    RenderSystem m_RenderSystem;

    // カラーターゲット
    ColorTarget m_SceneColorTarget;

	// 深度ターゲット
    DepthTarget m_SceneDepthTarget;

    // 定数バッファ
    ConstantBuffer m_TransformCB[FrameCount];
    ConstantBuffer m_CameraCB[FrameCount];
    ConstantBuffer m_MeshCB[FrameCount];

    // ImGui管理
	ImGuiUtil m_ImGuiUtil;

	// デバッグUI
	DebugUI m_DebugUI;

    // ライトの回転
    float m_RotateAngle;

    /// <summary>
	/// 初期化処理
    /// </summary>
    bool OnInit() override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void OnTerm() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void OnUpdate() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void OnRender() override;

    /// <summary>
	/// メッセージ処理
    /// </summary>
    void OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) override;

    /// <summary>
	///　シーンの描画
    /// </summary>
    void DrawScene(ID3D12GraphicsCommandList* pCmdList);

	/// <summary>
	/// RenderContextの作成
	/// </summary>
	RenderContext CreateRenderContext(ID3D12GraphicsCommandList* pCmdList);

    /// <summary>
	/// 影の描画対象オブジェクトの収集
    /// </summary>
    std::list<GameObject*> CollectShadowCasters(Scene* pScene);

	//=============================================================================
    //      描画処理
	//=============================================================================
    
    /// <summary>
	/// ライトマネージャーの更新
    /// </summary>
    void UpdateLightManager();

	/// <summary>
	/// シャドウマップのレンダリング
	/// </summary>
	void RenderShadowMap(ID3D12GraphicsCommandList* pCmdList);

    /// <summary>
	/// 定数バッファの更新
    /// </summary>
    void UpdateConstantBuffers();
};