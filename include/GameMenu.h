#pragma once

#include "GameObject.h"
#include <array>

class Sprite;

/// <summary>
/// インフィールド用のシステムメニュー（ポーズメニュー）。
/// 表示中は GameMenu::IsMenuOpen() が true。Scene 側で dt=0 停止などに利用する。
/// </summary>
class GameMenu : public GameObject
{
public:
	GameMenu() = default;
	~GameMenu() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	static bool IsMenuOpen();

private:
	// メニューのパネル
	enum class Panel : int
	{
		Main,
		ControlsHelp,
		SkillInfo,
	};

	// メインパネルの項目
	enum class MainItem : int
	{
		CloseMenu = 0,
		ReturnToTitle,
		Controls,
		SkillInfo,
		Count = 4,
	};

	// メニューを開く処理
	void OpenMenu();

	// メニューを閉じる処理
	void CloseMenu();

	// 操作方法
	void EnterControlsHelp();
	
	// スキル情報
	void EnterSkillInfo();
	
	// メインパネルに戻る
	void ReturnToMainPanel();

	// スプライトの表示・非表示やアルファ値の設定
	static void SetSpriteVisible(Sprite* sprite, bool visible);
	static void SetSpriteAlpha(Sprite* sprite, float alpha);

	// 選択中の項目の見た目を更新する（スケールや色を変える）
	void RefreshSelectVisual();

	// メニューの項目やスプライトの位置を、現在のパネルに応じたレイアウトに更新する
	void ApplyPanelLayout();

	// メニュー表示アニメーションの開始
	void BeginMenuIntro();

	// メニュー表示アニメーションの更新
	void UpdateMenuIntro(float deltaTime);
	
	// メニューの項目やスプライトの位置を、アニメーションの状態に応じて更新する
	void RestPositions();

	static bool s_MenuOpen; // メニューが開いているか(GameFlowUtil用)

	Sprite* m_ControlsHelpSprite = nullptr; // 操作方法のスプライト
	Sprite* m_SkillInfoSprite = nullptr;	// スキル情報のスプライト

	std::array<DirectX::XMFLOAT2, static_cast<size_t>(MainItem::Count)> m_MainItemRestPos{}; // メニューの項目の画面座標
	
	DirectX::XMFLOAT2 m_ControlsHelpRestPos{}; // 操作方法のスプライトの画面座標
	DirectX::XMFLOAT2 m_SkillInfoRestPos{};    // スキル情報のスプライトの画面座標

	float m_MenuIntroElapsed = 0.0f; // メニュー表示アニメーションの経過時間
	bool m_MenuIntroPlaying = false; // メニュー表示アニメーションが再生中かどうか

	static constexpr float kMenuIntroDurationSec = 0.5f; // メニュー表示アニメーションの時間
	static constexpr float kMenuSlideOffsetPx = 720.0f;  // メニュー表示アニメーションのスライド距離
	static constexpr float kSelectedScale = 1.1f; // 選択中の項目のスケール
	static constexpr float kNormalScale = 1.0f;	   // 選択されていない項目のスケール

	static constexpr XMFLOAT2 kPanelSize = { 600.0f, 400.0f }; // パネルのサイズ

	static constexpr float kMainItemBaseX = 180.0f; // メニュー項目の基本X座標
	static constexpr float kMainItemBaseY = 220.0f; // メニュー項目の基本Y座標
	static constexpr float kMainItemStepY = 120.0f; // メニュー項目のY座標の間隔

	bool m_IsMenuOpen = false;	 // メニューが開いているか
	Panel m_Panel = Panel::Main; // 現在のパネル
	int m_MainSelectIndex = 0;	 // メインパネルで現在選択されている項目のインデックス

	Sprite* m_Dim = nullptr; // メニューの背景を暗くするスプライト
	std::array<Sprite*, static_cast<size_t>(MainItem::Count)> m_MainItemSprites{};


};