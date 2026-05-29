#pragma once
#include "GameObject.h"
#include "PhaseData.h"
#include <vector>

class EnemySpawner;
class ShopManager;

// <summary>
// フェーズの状態
// </summary>
enum class PhaseState
{
	Idle,
	Spawning,	// エネミー出現中
	Fighting,	// 戦闘中
	PhaseClear, // フェーズクリア待ち
	Shopping,	// ショップ
	GameClear	// ゲームクリア
};

/// <summary>
// フェーズ管理クラス。敵のスポーンやフェーズの進行、ショップのインターバルなどを管理します。
/// </summary>
class PhaseManager : public GameObject
{
public:
	PhaseManager();
	~PhaseManager() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	int GetCurrentPhaseNumber() const;

	PhaseState GetPhaseState() const { return m_State; }
	bool IsGameClear() const { return m_State == PhaseState::GameClear; }
private:
	// フェーズの情報をJsonから取得
	void LoadPhaseData();

	//　現在のフェーズを開始します。エネミースポナーに敵の出現を指示します。
	void StartCurrentPhase();

	// 現在のフェーズで出現した敵が全て倒されたかどうかをチェックします。
	bool AreAllEnemiesDefeated() const;

	// 次のフェーズへ進みます。フェーズインデックスを更新し、必要に応じて設定を切り替えます。
	void AdvanceToNextPhase();

	// ショップ（アップグレード選択フェーズ）を開始します。
	void StartShopping();

	// ショップ中の更新処理。待機時間を管理し、制限時間を超過した場合は次のフェーズへ強制移行します。
	void UpdateShopping(float deltaTime);

	// 現在のクリアフェーズ状況に基づいて、ショップを開くべきかどうかを判定します。
	bool ShouldOpenShop() const;

	// 出現フェーズの開始
	void BeginSpawnPhase();
private:
	PhaseState m_State = PhaseState::Idle;      // 現在のフェーズ状態
	std::vector<PhaseData> m_Phases;            // 登録されているフェーズデータのリスト

	int m_CurrentPhaseIndex = 0;                // 現在進行中のフェーズ番号（インデックス）
	EnemySpawner* m_EnemySpawner = nullptr;     // 敵を出現させるスポナーのポインタ

	float m_PhaseClearTimer = 0.0f;             // 敵全滅から次のフェーズに進むまでの待機タイマー
	static constexpr float kPhaseClearWaitSec = 2.0f; // 敵全滅後の待機時間（秒）

	float m_SpawnDelayTimer = 0.0f;             // フェーズ開始から敵が出現するまでの遅延タイマー
	static constexpr float kSpawnDelaySec = 1.0f; // フェーズ開始から敵が出現するまでの遅延時間（秒）

	//-------------------------------------------------------------
	// ショップ関連
	ShopManager* m_ShopManager = nullptr;        // ショップ機能の管理インスタンス
	bool m_HasOpenedShop = false;                // すでにショップを開いたかどうかのフラグ
	static constexpr int kShopIntervalPhase = 5; // 何フェーズごとにショップを開くかの間隔
};