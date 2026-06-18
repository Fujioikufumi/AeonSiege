#include "CharacterBase.h"
#include "AnimationController.h"
#include "HealthComponent.h"
#include "MathUtility.h"
#include "DamageTypes.h"

void CharacterBase::ChangeAnimation(AnimationController* anim, const char* clipName, bool loop, float speed)
{
	if (anim == nullptr || !anim->IsInitialized() || clipName == nullptr) return;
	anim->SetLoop(loop);	// ループ設定
	anim->SetSpeed(speed);	// 再生速度設定
	anim->Play(clipName);	// アニメーション再生
}

void CharacterBase::SchedulePendingMeleeDamage(GameObject* target, int damage, float delaySec)
{
	m_HasPendingMeleeDamage = true;		// 近接ダメージ予約フラグを立てる
	m_PendingMeleeTarget = target;		// 対象を保持
	m_PendingMeleeDamage = damage;		// ダメージ量を保持
	m_MeleeDamageDelayTimer = delaySec; // 遅延時間を設定
}

void CharacterBase::UpdatePendingMeleeDamage(float deltaTime)
{
	// 近接ダメージ予約がない場合は何もしない
	if (!m_HasPendingMeleeDamage) return;

	// カウントダウン
	m_MeleeDamageDelayTimer -= deltaTime;
	if (m_MeleeDamageDelayTimer > 0.0f) return;

	// ダメージの適応
	m_HasPendingMeleeDamage = false;

	// ローカル変数に情報をコピーする
	GameObject* target	= m_PendingMeleeTarget;
	const int   damage	= m_PendingMeleeDamage;

	// 予約情報をクリアする
	m_PendingMeleeTarget = nullptr; // nullptrにする
	m_PendingMeleeDamage = 0;

	// 対象が有効かどうかをチェック
	if (target == nullptr || target->IsDestroyed()) return;

	// 対象のHealthComponentを取得して生存しているか確認
	HealthComponent* hp = target->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive()) return;

	// 近接ヒット判定距離内にいるかどうかをチェック
	if (!MathUtility::IsInRange(m_Position, target->GetPosition(), m_MeleeHitRange)) return;

	// ダメージを適用する
	DamageContext context{};
	context.attacker = this;
	context.damage = damage;
	target->ApplyDamage(context);
}

void CharacterBase::CancelPendingMeleeDamage()
{
	m_HasPendingMeleeDamage = false;
	m_MeleeDamageDelayTimer = 0.0f;
	m_PendingMeleeTarget = nullptr;
	m_PendingMeleeDamage = 0;
}