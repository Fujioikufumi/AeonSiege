#pragma once
#include "imgui.h"
#include "Camera.h"
#include "ModelManager.h"
#include "LightManager.h"
#include "Scene.h"
#include "NameSpace.h"
#include "GameObject.h"

enum class DebugWindowType
{
	Performance,
	Hierarchy,
	ImGuiDemo,
	ImGuiMetrics,
	Inspector,
	InspectorDetail,
	CombatHudLayout,
	Max
};

struct DebugWindowEntry
{
	std::string    name;
	bool           enable = false;
	DebugWindowType type;
};
class DebugUI
{
public:
	/// <summary>
	/// コンストラクタ・デストラクタ
	/// </summary>
	DebugUI();
	~DebugUI();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float deltaTime, uint32_t frameIndex, uint32_t width, uint32_t height);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// カメラ設定
	/// </summary>
	void SetCamera(Camera* camera) { m_pCamera = camera; }

	/// <summary>
	/// シーン設定
	/// </summary>
	void SetScene(Scene* scene) { m_pScene = scene; }

	/// <summary>
	/// 表示させるかどうかの設定・取得
	/// </summary>
	void SetVisible(bool visible) { m_Visible = visible; }
	bool IsVisible() const { return m_Visible; }

	/// <summary>
	/// アニメーション名の取得
	/// </summary>
	const std::string& GetSelectedAnimationName() const { return m_SelectedAnimationName; }

	/// <summary>
	/// 選択中モデルのパス取得
	/// </summary>
	const std::wstring& GetSelectedModelPath() const { return m_SelectedModelPath; }

	/// <summary>
	/// デバッグウィンドウ一覧（参考資料の m_windows。各ウィンドウの表示 on/off は enable で管理）
	/// </summary>
	std::vector<DebugWindowEntry> m_windows;

	/// <summary>
	/// ヒエラルキーで選択中のオブジェクト
	/// </summary>
	GameObject* m_pSelectedGameObject = nullptr;

	/// <summary>
	/// ヒエラルキーで選択中のゲームオブジェクトを取得
	/// </summary>
	GameObject* GetSelectedGameObject() const { return m_pSelectedGameObject; }

	/// <summary>
	/// 選択オブジェクトの詳細画面
	/// </summary>
	void DrawInspectorDetailWindow();
private:
	// 表示・非表示
	bool m_Visible = true;
	bool m_ShowPerformanceWindow = true;
	bool m_ShowModelManagerWindow = false;
	bool m_ShowSceneWindow = false;
	bool m_ShowCameraWindow = false;
	bool m_ShowLightWindow = false;
	bool m_ShowImGuiDemo = false;
	bool m_ShowImGuiMetrics = false;
	bool m_ShowTerrainWindow = false;

	bool m_ShowBoneHierarchyWindow = false;

	std::wstring m_SelectedModelPath;
	int m_SelectedModelIndex = -1;

	// アニメーション選択
	std::string m_SelectedAnimationName;
	int m_SelectedAnimationIndex = -1;

	float m_lastScale = 1.0f;

	// カメラ・シーン
	Camera* m_pCamera = nullptr;
	Scene* m_pScene = nullptr;

	/// <summary>
	/// ヒエラルキーウィンドウの描画
	/// </summary>
	void DrawHierarchyWindow();

	/// <summary>
	/// インスペクターウィンドウの描画
	/// </summary>
	void DrawInspectorWindow();

	/// <summary>
	/// 指定種類のウィンドウエントリを取得（メニューや ImGui の閉じる用に enable を渡すため）
	/// </summary>
	DebugWindowEntry* GetWindowEntry(DebugWindowType type);

	/// <summary>
	/// パフォーマンスウィンドウの描画
	/// </summary>
	void DrawPerformanceWindow(float deltaTime, uint32_t frameIndex, uint32_t width, uint32_t height);

	/// <summary>
	/// メインメニューバーの描画
	/// </summary>
	void DrawMainMenuBar();

	/// <summary>
	/// Combat HUD のレイアウトウィンドウの描画
	/// </summary>
	void DrawCombatHudLayoutWindow();
};