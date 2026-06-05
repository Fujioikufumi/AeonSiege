//-----------------------------------------------------------------------------
//  Include
//-----------------------------------------------------------------------------
#include "Player.h"
#include "ResourceManager.h"
#include "Input.h"
#include "GameManager.h"
#include <algorithm>
#include "NameSpace.h"
#include <vector>
#include "MathUtility.h"
#include "GameFlowUtil.h"
#include "SkillCatalog.h"
#include "CombatHud.h"
// ゲームオブジェクト
#include "Camera.h"
#include "Terrain.h"
// コンポーネント
#include "MeshRenderer.h"
#include "AnimationController.h"
#include "HealthComponent.h"
#include "TargetComponent.h"
#include "StatusComponent.h"

namespace {
	constexpr float kLockMaxDist = 400.0f;	// ロックオン可能な最大距離
	constexpr float kViewDotMin = 0.5f;		// ロックオン可能な視界の最低dot値（約60度）
	constexpr float kAShortTapMaxSec = 0.2f;// Aボタンの短押しと判定する最大時間
	constexpr float kALongPressSec = 0.8f;  // Aボタンの長押しと判定する時間
	constexpr float kStickRunThreshold = 0.85f; // スティックの入力が走りと判定される閾値（0.0f～1.0f、1.0fが最大入力）
	constexpr float kJumpPower = 3.0f; // ジャンプの初速
	constexpr float kGravity = -9.8f;	// 重力の強さ

	constexpr float kRotationSpeed = 10.0f;
	constexpr float kMoveEpsilon = 1.0e-6f;

	static constexpr WORD BTN_LOCK_ON = PAD_X;   // ロックオン（仕様に合わせて後で変更可）
	static constexpr WORD BTN_ACTION  = PAD_B;   // 戦闘入り短押し等（T / PAD_B）
	static constexpr WORD BTN_SKILL_1 = PAD_Y;   // スキル1
	static constexpr WORD BTN_SKILL_2 = PAD_X;   // スキル2
}


//-----------------------------------------------------------------------------
// 	    コンストラクタ
//-----------------------------------------------------------------------------
Player::Player()
{
	m_ModelPath = L"../Assets/Characters/MariaWProp.bmdl";
	m_PipelineName = L"SkinnedFBXPipeline";
	m_Position = { -497.0f, 0.0f, -650.0f };
	m_Scale = { 0.05f, 0.05f, 0.05f };
}

//-----------------------------------------------------------------------------
//          終了処理
//-----------------------------------------------------------------------------
Player::~Player()
{
	Term();
}

//-----------------------------------------------------------------------------
//          初期化処理
//-----------------------------------------------------------------------------
bool Player::Init()
{
	if(!GameObject::Init())
	{
		ELOG("Error : GameObject::Init() Failed in Player.cpp");
		return false;
	}

	// 初期状態は待機
	m_State = PlayerState::Idle;
	m_PrevState = PlayerState::Idle;

	// メッシュレンダラー
	AddComponent<MeshRenderer>()->Load(m_ModelPath, m_PipelineName);

	m_AnimController = AddComponent<AnimationController>();
	m_AnimController->SetLoop(true);
	m_AnimController->Play(kAnimIdle);

	// ステータスコンポーネントの追加
	m_Status = AddComponent<StatusComponent>();
	m_Status->Setup(CreateStatusData());

	// 体力管理コンポーネントの追加(ステータス設定よりも後で初期化)
	HealthComponent* healthComp = AddComponent<HealthComponent>();
	healthComp->SetMaxHP(m_Status->GetMaxHp());
	healthComp->SetHP(m_Status->GetMaxHp());

	SkillComponent* skills = AddComponent<SkillComponent>();
	skills->AddSkill(SkillCatalog::Create(SkillId::PlayerSlash1));

	return true;
}

//-----------------------------------------------------------------------------
// 		終了処理
//-----------------------------------------------------------------------------
void Player::Term()
{
	GameObject::Term();
}

//-----------------------------------------------------------------------------
// 		更新処理
//-----------------------------------------------------------------------------
void Player::Update(float deltaTime)
{
	Scene* scene = GameManager::GetScene();
	if (scene == nullptr)
	{
		GameObject::Update(deltaTime);
		return;
	}

	// 死亡チェック
	if (!m_IsDead)
	{
		HealthComponent* health = GetComponent<HealthComponent>();
		if (health && health->GetHP() <= 0)
		{
			m_IsDead = true;
			m_AnimController->SetLoop(false);
			m_AnimController->Play("Player_Combat_Dying");
		}
	}

	if (m_IsDead)
	{
		// 死亡アニメーション終了したかどうか
		if (!m_AnimController->IsPlaying())
		{
			m_IsDeadAnimFinished = true;
		}
		GameObject::Update(deltaTime);
		return;
	}

	if (GameFlowUtil::IsMenuOpen())
	{
		GameObject::Update(deltaTime);
		return;
	}

	Camera* camera = scene->GetGameObjectByName<Camera>("Camera");

	if (GameFlowUtil::IsShopOpen())
	{
		UpdateLockOnTarget(deltaTime, scene, camera);
		GameObject::Update(deltaTime);
		return;
	}

	UpdateHitEffect(deltaTime);
	UpdateLockOnTarget(deltaTime, scene, camera);

	Terrain* terrain = scene->GetGameObjectByName<Terrain>("Terrain");

	if (m_IsCombat)
	{
		UpdateCombatMode(deltaTime, scene, camera, terrain);
		UpdateAutoAttack(deltaTime);
	}
	else
	{
	}

	UpdateMovement(deltaTime, camera, terrain);
	UpdateSkillState(deltaTime);
	UpdateDamageDelay(deltaTime, camera);
	SyncCameraTarget(camera);

	GameObject::Update(deltaTime);
}

void Player::UpdateDamageDelay(float deltaTime, Camera* camera)
{
	if (m_DamageDelayTime <= 0.0f)
	{
		return;
	}

	m_DamageDelayTime = std::max(0.0f, m_DamageDelayTime - deltaTime);

	if (m_DamageDelayTime <= 0.0f && m_PlayingCombatAnim && m_LockTarget != nullptr)
	{
		CalculateAttackDamage(true, camera);
	}
}

void Player::SyncCameraTarget(Camera* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	// カメラの追従ターゲットをプレイヤーの位置に設定
	camera->SetTrackingTarget(GetPosition());

	// ロックオンターゲットがあればカメラの注視点をロックオンターゲットに設定
	TargetComponent* tComp = m_LockTarget ? m_LockTarget->GetComponent<TargetComponent>() : nullptr;
	if (tComp != nullptr)
	{
		camera->SetLookOnTarget(tComp->GetLockPosition());
	}
	else
	{
		// ロックオンターゲットがいない場合はカメラの注視点をクリア
		camera->ClearLookOnTarget();
	}
}


XMMATRIX Player::GetWorldMatrix() const
{
	XMMATRIX scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	XMMATRIX rot = XMMatrixRotationQuaternion(m_Quaternion);
	XMMATRIX trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	return scale * rot * trans;
}

XMFLOAT3 Player::GetForward() const
{
	XMMATRIX rot = XMMatrixRotationQuaternion(m_Quaternion);
	XMFLOAT3 forward;
	XMStoreFloat3(&forward, rot.r[2]);
	return forward;
}

float Player::GetSkillCooldownRatioBySlot(int slotIndex) const
{
	SkillComponent* skills = const_cast<Player*>(this)->GetComponent<SkillComponent>();
	return (skills != nullptr) ? skills->GetCooldownRatio(slotIndex) : 0.0f;
}
StatusData Player::CreateStatusData() const
{
	StatusData data;
	data.maxHp = kDefaultPlayerHP;
	data.moveSpeed = kMoveSpeed;
	data.attackPower = kAttackDamage;
	data.autoAttackInterval = kAutoAttackInterval;
	return data;
}

//-----------------------------------------------------------------------------
// 		戦闘アニメーションの開始
//-----------------------------------------------------------------------------
void Player::StartCombatAnim(const char* clipName, float speed)
{
	if (clipName == nullptr)
		return;

	if (m_AnimController->IsPaused())
		return;

	m_PlayingCombatAnim = true;
	m_AnimController->SetLoop(false);
	m_AnimController->SetSpeed(speed);
	m_AnimController->Play(clipName);
}
//-----------------------------------------------------------------------------
// 		戦闘アニメーションの更新
//-----------------------------------------------------------------------------
void Player::UpdateCombatAnim()
{
	if (!m_PlayingCombatAnim)
		return;
	if (m_AnimController->IsPaused())
		return;

	// 再生が終了しているかチェック
	if (m_AnimController->IsPlaying())
		return;

	m_PlayingCombatAnim = false;

	// 攻撃が終わった瞬間に、現在の m_IsCombat に基づいた最新の状態をセット
	if (m_IsCombat) {
		m_State = PlayerState::Combat;
	}
	else if (m_State == PlayerState::Combat) {
		m_State = PlayerState::Idle;
	}

	ResumeLocomotionAnim();
}

//-----------------------------------------------------------------------------
// 		戦闘アニメーションから通常アニメーションへの復帰
//-----------------------------------------------------------------------------
void Player::ResumeLocomotionAnim()
{
	m_AnimController->SetLoop(true);
	const char* clip = kAnimIdle;
	switch (m_State)
	{
	case PlayerState::Walk:   clip = kAnimWalk; break;
	case PlayerState::Run:    clip = kAnimRun; break;
	case PlayerState::Jump:   clip = kAnimJump; break;
	case PlayerState::Combat: clip = kAnimCombatIdle; break;
	case PlayerState::Idle:
	default:
		clip = kAnimIdle;
		break;
	}
	m_AnimController->Play(clip);
	m_PrevState = m_State;
}

//-------------------------------------------------------------
// 		アニメーション変更
//-------------------------------------------------------------
void Player::ChangeAnimation()
{
	if (m_PlayingCombatAnim)
	{
		return; // 戦闘アニメーション再生中は通常アニメーションに切り替えない
	}

	// 状態が変わっていなければ何もしない
	if (m_State == m_PrevState)
	{
		return;
	}

	const char* clipName = kAnimIdle;
	bool loop = true;

	switch (m_State)
	{
	case PlayerState::Idle:
		clipName = kAnimIdle;
		loop = true;
		break;
	case PlayerState::Walk:
		clipName = kAnimWalk;
		loop = true;
		break;
	case PlayerState::Run:
		clipName = kAnimRun;
		loop = true;
		break;
	case PlayerState::Jump:
		clipName = kAnimJump;
		loop = false; // ジャンプはループさせない
		break;
	case PlayerState::Combat:
		clipName = kAnimCombatIdle;
		loop = true;
		break;
	}
	m_AnimController->SetLoop(loop);
	m_AnimController->Play(clipName);
	m_PrevState = m_State;
}

//-----------------------------------------------------------------------------
// 		ロックオン対象が視界に入っているかどうか
//-----------------------------------------------------------------------------
bool Player::IsInSight(const Camera* camera, const GameObject* enemy) const
{
	const XMFLOAT3 playerPos = m_Position;
	DirectX::XMFLOAT3 ep = enemy->GetPosition();

	// 水平方向の距離を計算
	float dx = ep.x - playerPos.x;
	float dz = ep.z - playerPos.z;
	const float lenSq = dx * dx + dz * dz;

	// 至近距離の視界には入っているとみなす
	if (lenSq < 1.0e4f)
		return true;

	// 視界の判定
	const float invLen = 1.0f / sqrtf(lenSq);
	dx *= invLen;
	dz *= invLen;

	// カメラの前方向ベクトル（XZ平面）との内積を計算
	DirectX::XMFLOAT3 cf = camera->GetViewForwardXZ();
	const float dot = cf.x * dx + cf.z * dz;
	
	// dotがkViewDotMin以上なら視界に入っているとみなす
	return dot >= kViewDotMin;
}

//-----------------------------------------------------------------------------
// 		ロックオン対象の更新
//-----------------------------------------------------------------------------
void Player::UpdateLockOnTarget(float deltaTime, Scene* scene, Camera* camera)
{
	if (scene == nullptr)
	{
		return;
	}

	// ターゲットが有効かどうかの確認
	CheckLockTarget(scene);

	// 戦闘中かつロックオン対象が存在しない場合、ロックオン対象を探す。
	if (m_LockTarget == nullptr)
	{
		std::vector<GameObject*> candidates = CollectLockCandidates(scene, camera);
		if (!candidates.empty())
		{
			// 最も近い候補をロックオン対象にする
			m_LockTarget = candidates[0];
			m_IsCombat = true;
		}
		else
		{
			m_IsCombat = false;
		}
	}

	HandleLockOnInput(deltaTime, scene, camera);
	HandleCombatInput(deltaTime, scene, camera);
}

void Player::CheckLockTarget(Scene* scene)
{
	if (m_LockTarget == nullptr)
	{
		return;
	}

	// シーン上に存在するかを確認
	bool isInvalid = m_LockTarget->IsDestroyed() || !scene->ContainsGameObject(m_LockTarget);

	if (!isInvalid)
	{
		HealthComponent* hp = m_LockTarget->GetComponent<HealthComponent>();
		isInvalid = (hp == nullptr || !hp->IsAlive());
	}

	if (isInvalid)
	{
		m_LockTarget = nullptr;
	}
}

void Player::HandleLockOnInput(float deltaTime, Scene* scene, Camera* camera)
{
	// Yボタン短押しの場合、ロックオン対象の切り替え・長押しの場合、ロックオン解除
	const bool yHeld = IsControllerPress(BTN_LOCK_ON) || IsKeyPress('Y');
	if (yHeld)
	{
		m_YHoldTime += deltaTime;
	}
	else
	{
		// Yボタンが離されたとき
		if (m_WasYHeld)
		{
			if (m_YHoldTime >= kYHoldLimit)
			{
				// ロックオン解除
				m_LockTarget = nullptr;
			}
			else if (m_YHoldTime > 0.0f)
			{
				// ロックオン対象の切り替え
				SwitchTarget(scene, camera, 1);
			}
		}

		m_YHoldTime = 0.0f;
	}

	m_WasYHeld = yHeld;
}

void Player::HandleCombatInput(float deltaTime, Scene* scene, Camera* camera)
{
	// Aボタン又はTキーの入力判定
	const bool aHeld = IsControllerPress(BTN_ACTION) || IsKeyPress('T');
	
	if (aHeld)
	{
		m_AHoldTime += deltaTime;

		// 戦闘中かつAボタンが長押しされた場合、戦闘状態から抜ける
		if (m_IsCombat && !m_ExitCombat && m_AHoldTime >= kALongPressSec)
		{
			m_IsCombat = false;
			m_ExitCombat = true;
		}
	}
	else
	{
		if (m_WasAHeld)
		{
			// 戦闘状態でない、ロックオン対象が存在する、かつAボタンが短押しされた場合、戦闘状態に入る
			if (!m_IsCombat && m_LockTarget != nullptr && m_AHoldTime < kAShortTapMaxSec)
			{
				m_IsCombat = true;
			}
		}

		m_AHoldTime = 0.0f;
		m_ExitCombat = false;
	}

	m_WasAHeld = aHeld;
}

//-----------------------------------------------------------------------------
// 		ロックオン対象の候補を収集
//-----------------------------------------------------------------------------
std::vector<GameObject*> Player::CollectLockCandidates(const Scene* scene, const Camera* camera) const
{
	std::vector<GameObject*> out;
	if (scene == nullptr)
		return out;

	// シーンに存在する敵オブジェクトを収集
	const auto& enemyList = scene->GetGameObjectsByLayer(eLayer::ENEMY);

	for (const auto& up : enemyList)
	{
		GameObject* obj = up.get();
		if (obj == nullptr)
			continue;

		// ターゲットコンポーネントと体力コンポーネントを保持しているか、体力が残っているかの確認
		TargetComponent* targetComp = obj->GetComponent<TargetComponent>();
		HealthComponent* hp = obj->GetComponent<HealthComponent>();

		if (targetComp == nullptr || hp == nullptr || !hp->IsAlive())
			continue;

		// 距離の判定
		if (!MathUtility::IsInRange(m_Position, obj->GetPosition(), kLockMaxDist))
			continue;

		// 視界の判定
		if (!IsInSight(camera, obj))
			continue;

		// 全てを満たしたら候補に追加
		out.push_back(obj);
	}

	// 最後に距離の近い順にソート
	std::sort(out.begin(), out.end(),
		[this](GameObject* a, GameObject* b) {
			auto da = a->GetPosition();
			auto db = b->GetPosition();
			const float ax = da.x - m_Position.x, az = da.z - m_Position.z;
			const float bx = db.x - m_Position.x, bz = db.z - m_Position.z;
			return (ax * ax + az * az) < (bx * bx + bz * bz);
		});

	return out;
}

void Player::SwitchTarget(const Scene* scene, const Camera* camera, int direction)
{
	std::vector<GameObject*> candidates = CollectLockCandidates(scene, camera);
	if (candidates.empty())
		return;

	int idx = -1;
	for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
	{
		// 現在のロックオン対象が候補の配列のどこにいるかを探す
		if (candidates[i] == m_LockTarget)
		{
			idx = i;
			break;
		}
	}

	// 現在のロックオン候補数を取得
	const int n = static_cast<int>(candidates.size());
	
	if (n > 0)
	{
		// 次のインデックスの計算
		int nextIdx = (idx < 0) ? 0 : (idx + direction) % n;

		if (nextIdx < 0) nextIdx += n;
		m_LockTarget = candidates[nextIdx];
	}
}

//-----------------------------------------------------------------------------
// 		自動攻撃の更新
//-----------------------------------------------------------------------------
void Player::UpdateAutoAttack(float deltaTime)
{
	// 戦闘状態でない、ロックオン対象がいない、ロックオン対象が死んでいる場合は何もしない
	if (!m_IsCombat)
		return;

	if (m_LockTarget == nullptr)
		return;

	HealthComponent* lockTargetHealth = m_LockTarget->GetComponent<HealthComponent>();
	if (lockTargetHealth == nullptr || !lockTargetHealth->IsAlive())
		return;

	// クールダウンの更新
	m_CurrentAutoAtkCd = std::max(0.0f, m_CurrentAutoAtkCd - deltaTime);

	// 自動攻撃の発動判定
	if (!MathUtility::IsInRange(m_Position, m_LockTarget->GetPosition(), kAutoAttackRange))
		return;

	if (m_CurrentAutoAtkCd > 0.0f)
		return; // クールダウン中なら何もしない

	if (m_PlayingCombatAnim)
		return;// 戦闘アニメーション再生中なら何もしない

	m_DamageDelayTime = kAttackDamageDelay;
	m_PendingDamage = m_Status->GetAttackPower();

	// 攻撃アニメーションの開始
	StartCombatAnim(kAnimAttackAuto);
	
	m_CurrentAutoAtkCd = m_Status->GetAutoAttackInterval();
}

//-----------------------------------------------------------------------------
// 		スキルの更新
//-----------------------------------------------------------------------------
void Player::UpdateSkills(float deltaTime)
{
	SkillComponent* skills = GetComponent<SkillComponent>();
	if (skills == nullptr)
	{
		return;
	}


	if (!m_IsCombat || m_LockTarget == nullptr)
	{
		return;
	}

	if (IsKeyTrigger('1')) TryStartSkillSlot(0);
	if (IsKeyTrigger('2')) TryStartSkillSlot(1);
	if (IsKeyTrigger('3')) TryStartSkillSlot(2);
	if (IsKeyTrigger('4')) TryStartSkillSlot(3);
	if (IsKeyTrigger('5')) TryStartSkillSlot(4);
	if (IsKeyTrigger('6')) TryStartSkillSlot(5);
}

void Player::UpdateSkillState(float deltaTime)
{
	m_CurrentChainTime = std::max(0.0f, m_CurrentChainTime - deltaTime);

	UpdateSkills(deltaTime);
	UpdateActiveSkill(deltaTime);
	UpdateCombatAnim();
	ChangeAnimation();
}

bool Player::TryStartSkillSlot(int slotIndex)
{
	SkillComponent* skills = GetComponent<SkillComponent>();
	if (skills == nullptr)
	{
		return false;
	}

	SkillRuntime* skill = skills->GetSkillBySlot(slotIndex);
	if (skill == nullptr || skill->CurrentCooldown > 0.0f)
	{
		// スキルがクールダウン中なら発動しない
		return false;
	}

	if (m_LockTarget == nullptr || m_LockTarget->IsDestroyed())
	{
		// ロックオン対象が存在しない場合も発動しない
		return false;
	}

	HealthComponent* hp = m_LockTarget->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive())
	{
		return false;
	}
	// 発動するスキルの射程を取得
	const float skillRangeRate = m_Status->GetSkillRangeRate();

	const float useRange = skill->Data.range * skillRangeRate;

	// ロックオン対象が射程内にいるかの判定
	if (!MathUtility::IsInRange(m_Position, m_LockTarget->GetPosition(), useRange))
	{
		return false;
	}

	m_DamageDelayTime = 0.0f;	// ダメージタイマーのリセット 
	m_PendingDamage = 0;		// 与えるスキルのダメージをリセット

	// スキルのアニメーションを再生
	StartCombatAnim(skill->Data.animationName);

	m_HasActiveSkill = true;		// 発動中にする
	m_ActiveSkill = skill->Data;	// 発動するスキルのデータを保存
	m_SkillTarget = m_LockTarget; // スキルの対象を設定
	m_ActiveSkillTime = skill->Data.effectDelaySec;
	m_ActiveSkillHitCount = 0;

	// スキルの攻撃倍率を初期化（チェインダメージがない場合は1.0fで後にチェイン判定で使用する）
	m_ActiveSkillDamageRate = 1.0f;

	// チェインタイマーが残っているか
	if (m_CurrentChainTime > 0.0f)
	{
		// スキルの攻撃倍率にチェインダメ―ジを加算
		m_ActiveSkillDamageRate = m_Status->GetSkillChainDamageRate();

		// チェインタイマーをリセット
		m_CurrentChainTime = 0.0f;
	}

	// 発動したスキルのクールダウンを開始
	const float cooldownSec = skill->Data.cooldownSec;

	skills->StartCooldown(slotIndex, cooldownSec);
	return true;
}

void Player::UpdateActiveSkill(float deltaTime)
{
	if (!m_HasActiveSkill)
	{
		// 発動中のスキルがない場合は何もしない
		return;
	}

	// スキルの効果発生までの遅延時間をカウントダウン
	m_ActiveSkillTime -= deltaTime;
	if (m_ActiveSkillTime > 0.0f)
	{
		return;
	}

	ApplyActiveSkillHit();

	++m_ActiveSkillHitCount;
	if (m_ActiveSkillHitCount >= m_ActiveSkill.hitCount)
	{
		m_HasActiveSkill = false;
		m_SkillTarget = nullptr;
		return;
	}

	m_ActiveSkillTime = m_ActiveSkill.hitIntervalSec;
}

void Player::ApplyActiveSkillHit()
{
	CalculateAttackDamage(false, nullptr);
}

bool Player::ApplySkillDamage(GameObject* target, const DamageContext& context)
{
	if (target == nullptr || target->IsDestroyed())
	{
		return false;
	}

	HealthComponent* hp = target->GetComponent<HealthComponent>();
	if (hp == nullptr || !hp->IsAlive())
	{
		return false;
	}

	DamageResult result = target->ApplyDamage(context);

	if(result.hit)
	{
		GameManager::GetStatus().totalDamage += result.finalDamage;
	}

	return result.hit;
}

std::vector<GameObject*> Player::CollectSkillTargets(const SkillData& skill) const
{
	std::vector<GameObject*> targets;

	Scene* scene = GameManager::GetScene();
	if (scene == nullptr)
	{
		return targets;
	}

	const auto& enemies = scene->GetGameObjectsByLayer(eLayer::ENEMY);
	const float skillRangeRate = m_Status->GetSkillRangeRate();
	const float areaRadius = skill.areaRadius * skillRangeRate;
	const float range = skill.range * skillRangeRate;
	for (const auto& enemy : enemies)
	{
		GameObject* obj = enemy.get();
		if (obj == nullptr || obj->IsDestroyed())
		{
			continue;
		}

		HealthComponent* hp = obj->GetComponent<HealthComponent>();
		if (hp == nullptr || !hp->IsAlive())
		{
			continue;
		}

		switch (skill.targetType)
		{
		case SkillTargetType::SingleTarget:
			if (obj == m_SkillTarget)
			{
				targets.push_back(obj);
			}
			break;

		case SkillTargetType::AroundSelf:
			if (MathUtility::IsInRange(m_Position, obj->GetPosition(),	areaRadius))
			{
				targets.push_back(obj);
			}
			break;

		case SkillTargetType::AroundTarget:
			if (m_SkillTarget != nullptr &&
				MathUtility::IsInRange(m_SkillTarget->GetPosition(), obj->GetPosition(), areaRadius))
			{
				targets.push_back(obj);
			}
			break;

		case SkillTargetType::ForwardCone:
			if (IsInForwardCone(obj, range, skill.coneAngleDeg))
			{
				targets.push_back(obj);
			}
			break;
		}
	}

	return targets;
}

bool Player::IsInForwardCone(const GameObject* target, float range, float angleDeg) const
{
	if (target == nullptr)
	{
		return false;
	}

	const XMFLOAT3 targetPos = target->GetPosition();
	const float dx = targetPos.x - m_Position.x;
	const float dz = targetPos.z - m_Position.z;
	const float distSq = dx * dx + dz * dz;
	if (distSq > range * range || distSq <= 0.0001f)
	{
		return false;
	}

	XMFLOAT3 forward = GetForward();
	const float fLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
	const float tLen = std::sqrt(distSq);
	if (fLen <= 0.0001f || tLen <= 0.0001f)
	{
		return false;
	}

	const float fx = forward.x / fLen;
	const float fz = forward.z / fLen;
	const float tx = dx / tLen;
	const float tz = dz / tLen;

	const float dot = fx * tx + fz * tz;
	const float halfRad = DirectX::XMConvertToRadians(angleDeg * 0.5f);
	const float minDot = std::cos(halfRad);

	return dot >= minDot;
}

void Player::CalculateAttackDamage(bool isAutoAttack, Camera* camera)
{
	DamageContext context;
	context.attacker = this;

	// コンボの判定 : オートアタックではない、かつスキルダメージ倍率が1.0fを超えている場合
	context.isCombo = !isAutoAttack && (m_ActiveSkillDamageRate > 1.0f);

	if (isAutoAttack)
	{
		// オートアタックのダメージは、基本攻撃力 x ステータスの攻撃力倍率
		context.damage = m_Status->GetAttackPower();
	}
	else
	{
		// スキルダメージの計算
		const int skillDamage = m_Status->CalculateSkillDamage(m_ActiveSkill.skillPowerRate);

		// チェインダメージの適用 (チェインではない場合には1.0fが代入されている)
		context.damage = static_cast<int>(skillDamage * m_ActiveSkillDamageRate);
	}

	// クリティカルの判定
	const float criticalRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	if (criticalRoll < m_Status->GetCriticalRate())
	{
		// クリティカルヒットの場合、ダメージにクリティカル倍率を適応
		context.damage = static_cast<int>(
			static_cast<float>(context.damage) * m_Status->GetCriticalDamageRate()
			);
		context.isCritical = true;
	}
	if (isAutoAttack && camera != nullptr)
	{
		//------------------------------------------------------------------------------
		// オートアタックのダメ―ジ計算
		if (m_LockTarget == nullptr)
		{
			return;
		}

		// ダメージを与える
		DamageResult result = m_LockTarget->ApplyDamage(context);
		if (result.hit)
		{
			GameManager::GetStatus().totalDamage += result.finalDamage;
			// ここでヒットエフェクトを発動
			TriggerEnemyHitEffect(camera,
				kEnemyHitImpactHoldAuto, kEnemyHitShakeMag, kEnemyHitShakeDuration);
			m_CurrentChainTime = kChainSec;
		}
		return;
	}

	//------------------------------------------------------------------------------
	//	スキル攻撃

	std::vector<GameObject*> targets = CollectSkillTargets(m_ActiveSkill);
	bool hitAny = false;
	for (GameObject* target : targets)
	{
		if (ApplySkillDamage(target, context))
		{
			hitAny = true;
		}
	}
	if (hitAny)
	{
		TriggerEnemyHitEffect(camera,
			kEnemyHitImpactHoldAuto, kEnemyHitShakeMag, kEnemyHitShakeDuration);
		m_CurrentChainTime = kChainSec;
	}
}

//-----------------------------------------------------------------------------
// 		戦闘モードの更新
//-----------------------------------------------------------------------------
void Player::UpdateCombatMode(float deltaTime, Scene* scene, Camera* camera, Terrain* terrain)
{
	// ターゲット変更処理を SwitchToNextLockTarget に委譲して超スッキリ！
	if (scene == nullptr || camera == nullptr)
		return;

	// 十字キー・左方向キーで前の敵
	if (IsControllerTrigger(PAD_LEFT) || IsKeyTrigger(VK_LEFT))
	{
		SwitchTarget(scene, camera, -1);
	}
	// 十字キー・右方向キーで次の敵
	else if (IsControllerTrigger(PAD_RIGHT) || IsKeyTrigger(VK_RIGHT))
	{
		SwitchTarget(scene, camera, 1);
	}
}

//-----------------------------------------------------------------------------
// 		プレイヤーの移動更新
//-----------------------------------------------------------------------------
void Player::UpdateMovement(float deltaTime, Camera* camera, Terrain* terrain)
{
	bool isPlayingSkill = false;

	// --- スキル発動中は移動やジャンプを完全にロックする ---
	if (m_AnimController != nullptr && m_AnimController->IsInitialized() && m_PlayingCombatAnim)
	{
		// 現在の再生中のアニメーションがスキルかどうかを名前で判定
		const std::string clipName = m_AnimController->GetCurrentAnimationName();
		isPlayingSkill = std::string_view(clipName).starts_with("Player_Skill");

		// オートアタック中は移動入力でキャンセル可能にする
		const float stickMagSq = GetLeftStickX() * GetLeftStickX() + GetLeftStickY() * GetLeftStickY();
		const bool hasMoveInput = IsKeyPress('W') || IsKeyPress('S') || IsKeyPress('A') || IsKeyPress('D') || (stickMagSq > 0.01f);

		if (hasMoveInput && clipName == kAnimAttackAuto)
		{
			m_PlayingCombatAnim = false;
			m_PrevState = PlayerState::Idle;
		}
	}

	// --------------------------------------------------------
	// 1. 入力ベクトルの計算
	// --------------------------------------------------------
	XMFLOAT3 fwd = { 0.0f, 0.0f, 1.0f };
	XMFLOAT3 right = { 1.0f, 0.0f, 0.0f };
	if (camera != nullptr)
	{
		fwd = camera->GetViewForwardXZ();
		right = camera->GetViewRightXZ();
	}

	float mx = 0.0f;
	float mz = 0.0f;

	// スキルアニメション中は移動不可
	if (!isPlayingSkill)
	{
		if (IsKeyPress('W')) { mx += fwd.x; mz += fwd.z; }
		if (IsKeyPress('S')) { mx -= fwd.x; mz -= fwd.z; }
		if (IsKeyPress('D')) { mx += right.x; mz += right.z; }
		if (IsKeyPress('A')) { mx -= right.x; mz -= right.z; }

		mx += GetLeftStickX() * right.x + GetLeftStickY() * fwd.x;
		mz += GetLeftStickX() * right.z + GetLeftStickY() * fwd.z;
	}

	// 入力があるかどうかの判定
	const float rawLenSq = mx * mx + mz * mz;
	const bool bMove = (rawLenSq > kMoveEpsilon);

	const float stickMagSq = GetLeftStickX() * GetLeftStickX() + GetLeftStickY() * GetLeftStickY();

	// ダッシュの判定:Shiftキー、スティックが一定以上倒している場合
	const bool bRun = IsKeyPress(VK_SHIFT)
		|| IsControllerPress(PAD_RIGHT_SHOULDER)
		|| (stickMagSq >= kStickRunThreshold * kStickRunThreshold);

	// --------------------------------------------------------
	// 2. 移動と回転の適用
	// --------------------------------------------------------

	float targetYaw = camera ? std::atan2(fwd.x, fwd.z) : std::atan2(GetForward().x, GetForward().z);

	// 移動の適応
	if (bMove)
	{
		const float rawLen = std::sqrt(rawLenSq);
		const float nx = mx / rawLen;
		const float nz = mz / rawLen;
		const float analog = std::min(rawLen, 1.0f);

		const float baseMoveSpeed = m_Status ? m_Status->GetMoveSpeed() : kMoveSpeed;
		const float speed = baseMoveSpeed * (bRun ? kRunMoveRate : 1.0f);
		const float step = speed * analog * deltaTime;

		m_Position.x += nx * step;
		m_Position.z += nz * step;

		targetYaw = std::atan2(nx, nz);
	}

	// 回転の適用（スキル中は現在の向きを保持）
	if (!isPlayingSkill)
	{
		const float currentYaw = std::atan2(GetForward().x, GetForward().z);
		const float newYaw = MathUtility::SlerpYaw(currentYaw, targetYaw, kRotationSpeed, deltaTime);
		m_Quaternion = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), newYaw);
		m_Quaternion = XMQuaternionNormalize(m_Quaternion);
	}

	// --------------------------------------------------------
	//	3.ジャンプ
	// --------------------------------------------------------

	// 地面の高さを取得
	const float fGroundY = terrain ? terrain->GetHeightAt(m_Position.x, m_Position.z) : 0.0f;

	if (m_IsGrounded && m_VelocityY <= 0.0f)
	{
		m_Position.y = fGroundY;
	}

	// 戦闘中はジャンプを無効化
	const bool bJump = !m_IsCombat && (IsKeyTrigger(VK_SPACE) || IsControllerTrigger(PAD_A));
	
	// ジャンプの開始
	if (m_IsGrounded && bJump)
	{
		m_VelocityY = kJumpPower;
		m_IsGrounded = false;
	}

	if (!m_IsGrounded || m_VelocityY > 0.0f)
	{	
		// ジャンプ中の重力の適用
		m_VelocityY += kGravity * deltaTime;
		m_Position.y += m_VelocityY * deltaTime;
	}

	if (m_Position.y <= fGroundY)
	{
		// 地形に接地している場合
		m_Position.y = fGroundY;
		m_VelocityY = 0.0f;
		m_IsGrounded = true;
	}
	else
	{
		m_IsGrounded = false;
	}

	// --------------------------------------------------------
	// 4. アニメーション状態の更新
	// --------------------------------------------------------
	// スキルアニメーションなどの CombatAnime が再生されている場合は、通常の状態遷移を行わない
	PlayerState nextState = m_State;
	if (!m_IsGrounded)
		nextState = PlayerState::Jump;
	else if (bMove && bRun)
		nextState = PlayerState::Run;
	else if (bMove)
		nextState = PlayerState::Walk;
	else if (m_IsCombat)
		nextState = PlayerState::Combat;
	else
		nextState = PlayerState::Idle;

	if (!m_PlayingCombatAnim)
	{
		m_State = nextState;
	}
	else
	{
		// 攻撃中でも内部的な状態だけは更新しておく（ResumeLocomotionAnim用）
		m_State = nextState;
	}
}

void Player::TriggerEnemyHitEffect(Camera* camera, float hitHoldSec, float shakeMag, float shakeDurSec)
{
	if (camera != nullptr)
		camera->StartHitImpactShake(shakeMag, shakeDurSec);
	if (!m_PlayingCombatAnim)
		return;
	m_AnimController->Pause();
	m_HitPause = std::max(m_HitPause, hitHoldSec);
}

void Player::UpdateHitEffect(float deltaTime)
{
	if (m_HitPause <= 0.0f)
		return;
	m_HitPause -= deltaTime;
	if (m_HitPause <= 0.0f && m_AnimController->IsPaused())
		m_AnimController->Resume();
}
