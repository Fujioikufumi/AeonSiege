#pragma once
#include "Core/GameObject.h"

class StatusComponent;
class AnimationController;

/// <summary>
/// 戦闘キャラクター共通の基底クラス。
/// 敵・味方が共通で持つステータス参照、アニメ切り替え、
/// 遅延近接ダメージ処理をまとめる。
/// </summary>
class CharacterBase : public GameObject
{
public:
	CharacterBase()           = default;
	~CharacterBase() override = default;

protected:
	//-------------------------------------------------------------
	// アニメーション

	/// アニメーションクリップを切り替える（未初期化なら何もしない）
	void ChangeAnimation(AnimationController* anim, const char* clipName,
	                     bool loop, float speed = 1.0f);

	//-------------------------------------------------------------
	// 遅延近接ダメージ（アニメのヒットタイミングに合わせて発生させる）

	/// 近接ダメージを予約する（delaySec 後に judge して適用）
	void SchedulePendingMeleeDamage(GameObject* target, int damage, float delaySec);

	/// 予約済みの近接ダメージを進行・適用する（毎フレーム呼ぶ）
	void UpdatePendingMeleeDamage(float deltaTime);

	/// 予約済みの近接ダメージを破棄する
	void CancelPendingMeleeDamage();

protected:
	StatusComponent* m_Status = nullptr; // ステータス管理コンポーネント

	// 近接ヒット判定距離（派生クラスが Init で設定する）
	float m_MeleeHitRange = 0.0f;

private:
	// 遅延近接ダメージの内部状態
	bool m_HasPendingMeleeDamage     = false;
	float m_MeleeDamageDelayTimer    = 0.0f;
	GameObject* m_PendingMeleeTarget = nullptr;
	int m_PendingMeleeDamage         = 0;
};