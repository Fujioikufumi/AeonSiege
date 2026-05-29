#pragma once
#include <string>
#include <vector>
#include <array>
#include "NameSpace.h"

namespace HudLayoutUtil
{
	inline float ScreenWidthRatio(float t) { return static_cast<float>(SCREEN_WIDTH) * t; }
	inline float ScreenHeightRatio(float t) { return static_cast<float>(SCREEN_HEIGHT) * t; }
}

// 画面比率に基づく矩形情報
struct HudBox {
	// 0.0 ~ 1.0 の比率で指定
	float cx = 0.5f;
	float cy = 0.5f;
	float w = 0.1f;
	float h = 0.1f;
};

// 数値UIの配置情報
struct HudNumberLayout {
	// 数値の配置
	float onesDigitX = 0.5f;
	float onesDigitY = 0.5f;
	float scale = 0.35f;
};

// バー表示の設定（HP/経験値などで使用）
struct HudBarLayout {
	HudBox barBack; // 背景
	HudBox bar;     // 減少するバー (背景の座標とスケールは一致させる)
	std::array<float, 3> colBack = { 0.3f, 0.3f, 0.3f }; // 背景の色
	std::array<float, 3> colBar = { 0.2f, 0.9f, 0.3f };  // バーの色
	HudNumberLayout number; // 表示する数値のレイアウト
};

// 味方1人分のレイアウトデータ
struct AllyLayout {
	std::string hpFrom;         // 対象オブジェクト名 (例: "Player")
	std::wstring panelPath;     // キャラクターパネル用テクスチャ
	bool showHpNum = true;      // HP数値の表示有無
	HudBox panel;				// キャラクターごとの背景パネル
	HudBarLayout hp;			// HPバーのレイアウト
	HudBarLayout exp;			// 経験値バーのレイアウト
	HudNumberLayout levelNumber;// レベル表示のレイアウト
};

// スキル表示のレイアウト
struct SkillLayout {
	int cdSlot = 0; // スキルスロット
};

// HUD全体の共通レイアウト設定
struct CombatHudLayout {
	// 敵HP関連
	HudBox enemyDecor;
	HudBox enemyBar;
	std::array<float, 3> colGray = { 0.45f, 0.45f, 0.48f };
	std::array<float, 3> colHp = { 0.95f, 0.15f, 0.12f };
	std::array<float, 3> colHpLag = { 0.75f, 0.12f, 0.10f };

	// スキル全体設定
	float skillCenterX = 0.5f;
	float skillY = 0.86f;
	float skillH = 0.075f;
	float skillGap = 0.065f;

	std::vector<AllyLayout> allies;
};

// 定数
constexpr int kMaxAllies = 2;
constexpr int kMaxSkills = 8;
constexpr int kHudMaxHpDisplay = 99999; // HP表示の最大値
constexpr int kHudMaxLevelDisplay = 99; // レベル表示の最大値