#pragma once

class GameObject;

/// <summary>
/// ダメージ処理に必要な情報をまとめた構造体
/// </summary>
struct DamageContext
{
	int damage = 0;					// 攻撃側の最終ダメージ
	GameObject* attacker = nullptr; // 攻撃側のオブジェクト
	bool isCritical = false;		// クリティカルかどうか
	bool isCombo = false;			// コンボかどうか
};

/// <summary>
/// ダメージ処理の結果をまとめた構造体
/// </summary>
struct DamageResult
{
	bool hit = false;		// 命中したかどうか
	bool evaded = false;	// 回避されたかどうか
	bool critical = false;	// クリティカルかどうか
	int finalDamage = 0;	// 最終ダメージ量
};