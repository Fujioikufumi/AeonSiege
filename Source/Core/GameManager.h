#pragma once
#include <functional>
#include "RenderSystem.h"
#include "Scene.h"

/// <summary>
/// ゲームのリザルトで表示させる情報
/// </summary>
struct GameStatus
{
	int killCount = 0;
	int totalDamage = 0;
	int currentPhase = 0;
};

/// <summary>
/// シーンを管理するクラス。シーンの更新や描画処理を実行する
/// </summary>
class GameManager
{
private:
	// スマートポインターを使用する
	static std::unique_ptr<Scene> m_ActiveScene;
	static std::unique_ptr<Scene> m_NextScene;
	static bool m_IsChangeScene;
	static GameStatus m_Status;
public:
	static void Init();
	static void Term();
	static void Update(float deltaTime);
	static void Draw(const RenderContext& context, ID3D12GraphicsCommandList* pCmdList, D3D12_CPU_DESCRIPTOR_HANDLE colorRTV);

	static GameStatus& GetStatus() { return m_Status; }
	static void ResetGameStatus() { m_Status = {}; }

	static void ChangeScene(Scene* newScene);
	static Scene* GetScene() { return m_ActiveScene.get(); }
};