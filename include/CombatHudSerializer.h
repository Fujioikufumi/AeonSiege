#pragma once
#include "CombatHudLayout.h"

/// <summary>
/// 戦闘中UIのレイアウトデータをJsonから読み込むシリアライザー
/// </summary>
class CombatHudSerializer {
public:
    // Jsonファイルの読み込み
    static bool Load(const std::wstring& path, CombatHudLayout& outLayout);

	// Jsonファイルへの書き出し
    static bool Save(const std::wstring& path, const CombatHudLayout& layout);
    
	// デフォルト値を設定する（Loadの先頭で呼び出される）
    static void ApplyDefaults(CombatHudLayout& out);
};