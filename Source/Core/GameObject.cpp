#include "GameObject.h"
#include "HealthComponent.h"
#include "StatusComponent.h"
#include "FloatingDamage.h"
#include "MathUtility.h"
#include "GameManager.h"
#include "Scene.h"
#include "Camera.h"
#include "DamageTypes.h"
#include "PhaseManager.h"
#include "PartyManager.h"
#include "EnemyAIComponent.h"
#include <cstdlib>
#include <algorithm>

namespace
{
	// ダメージテキストを表示する高さのオフセット
	static constexpr float kDamageTextOffsetY = 3.0f;

	// 表示するダメージテキストの種類を引数から判別する関数
	static FloatingDamageType ResolveFloatingDamageType(const DamageContext& context, const DamageResult& result)
	{
		if (result.evaded)
		{
			return FloatingDamageType::Miss; // 回避した
		}
		if (context.isCombo && result.critical)
		{
			return FloatingDamageType::ComboCritical; // コンボ & クリティカル
		}
		if (context.isCombo)
		{
			return FloatingDamageType::ComboDamage; // コンボ
		}
		if (context.attacker->GetLayer() == eLayer::ALLY || 
			context.attacker->GetLayer() == eLayer::PLAYER)
		{
			return result.critical
				? FloatingDamageType::EnemyCritical // 敵に対してのクリティカル
				: FloatingDamageType::EnemyDamage;	// 敵に対しての通常ダメージ
		}
		return result.critical
			? FloatingDamageType::AllyCritical // 味方に対してのクリティカル
			: FloatingDamageType::AllyDamage;  // 味方に対しての通常ダメージ
	}

	// ダメージテキストを生成してシーンに追加する関数
	static void SpawnFloatingDamage(GameObject* target, const DamageContext& context, const DamageResult& result)
	{
		if (target == nullptr)
		{
			return;
		}

		Scene* scene = GameManager::GetScene();
		if (scene == nullptr)
		{
			return;
		}

		Camera* camera = scene->GetGameObjectByName<Camera>("Camera");
		if (camera == nullptr)
		{
			return;
		}

		DirectX::XMFLOAT3 pos = target->GetPosition();
		pos.y += kDamageTextOffsetY;

		DirectX::XMFLOAT2 screenPos = MathUtility::WorldToScreen(pos, camera);

		FloatingDamageType type = ResolveFloatingDamageType(context, result);

		FloatingDamage* damageText = scene->AddGameObject<FloatingDamage>(eLayer::UI, "FloatingDamage");
		if (damageText != nullptr)
		{
			damageText->Setup(result.finalDamage, screenPos.x, screenPos.y, type);
		}
	}
}

//----------------------------------------------------------------------
//		コンストラクタ
//----------------------------------------------------------------------
GameObject::GameObject()
{
}

//----------------------------------------------------------------------
//		デストラクタ
//----------------------------------------------------------------------
GameObject::~GameObject()
{
	Term();
}

//----------------------------------------------------------------------
//		初期化処理
//----------------------------------------------------------------------
bool GameObject::Init()
{
	for (const auto& comp : m_Components)
	{
		comp->Init();
	}
	return true;
}

//----------------------------------------------------------------------
//		終了処理
//----------------------------------------------------------------------
void GameObject::Term()
{
	for (const auto& comp : m_Components)
	{
		comp->Term();
	}
	m_Components.clear();
}

//----------------------------------------------------------------------
//		更新処理
//----------------------------------------------------------------------
void GameObject::Update(float deltaTime)
{
	UpdateWorldMatrix();
	for (const auto& comp : m_Components)
	{
		comp->Update(deltaTime);
	}
}

//----------------------------------------------------------------------
//		描画処理
//----------------------------------------------------------------------
void GameObject::Draw(const RenderContext& context)
{
	for (const auto& comp : m_Components)
	{
		comp->Draw(context);
	}
}

//----------------------------------------------------------------------
// 
//----------------------------------------------------------------------
void GameObject::UpdateWorldMatrix()
{
	 m_WorldMatrix = GetWorldMatrix();
}

//----------------------------------------------------------------------
// 		ワールド行列の取得
//----------------------------------------------------------------------
XMMATRIX GameObject::GetWorldMatrix() const
{
	const XMMATRIX scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	const XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	const XMMATRIX trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	return scale * rot * trans;
}

//----------------------------------------------------------------------
// 		前方向の取得
//----------------------------------------------------------------------
XMFLOAT3 GameObject::GetForward() const
{
	XMMATRIX rotationMatrix;
	rotationMatrix = XMMatrixRotationRollPitchYaw(m_Rotation.x,	m_Rotation.y, m_Rotation.z);
	
	XMFLOAT3 forward;
	XMStoreFloat3(&forward, rotationMatrix.r[2]);
	
	return forward;
}

DamageResult GameObject::ApplyDamage(const DamageContext& context)
{
	DamageResult result;

	// 自身のステータスを取得 (攻撃を受ける側)
	StatusComponent* defenderStatus = GetComponent<StatusComponent>();
	
	// ステータスが存在する場合、回避の判定を行う
	if (defenderStatus != nullptr)
	{
		// 回避の判定
		const float evasionRate = defenderStatus->GetEvasionRate();
		
		// 0.0 ~ 1.0 で乱数を生成
		const float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		if (roll < evasionRate)
		{
			result.evaded = true; // 回避の成功
			SpawnFloatingDamage(this, context, result); // 回避UIの表示
			return result;
		}
	}

	// ダメージの計算
	int damage = context.damage;
	if (defenderStatus != nullptr)
	{
		// ダメージ減少の計算
		damage = defenderStatus->CalculateTakenDamage(damage);
	}

	// 体力コンポーネントにダメージを適用
	HealthComponent* hp = GetComponent<HealthComponent>();
	if (hp != nullptr && hp->IsAlive())
	{
		hp->ApplyDamage(damage);
		result.hit = true;

		if (!hp->IsAlive())
		{
			// エネミーが撃破されたらパーティに経験値を与える
			Scene* scene = GameManager::GetScene();
			EnemyAIComponent* eai = GetComponent<EnemyAIComponent>();
			PartyManager* pm = (scene != nullptr) ? scene->GetGameObjectByName<PartyManager>("PartyManager") : nullptr;
			if (pm != nullptr && eai != nullptr)
				pm->AddExpToAllies(eai->GetExpReward());
		}
	}
	result.critical = context.isCritical;	// クリティカル判定
	result.finalDamage = damage;			// 最終ダメージ
	SpawnFloatingDamage(this, context, result); // ダメージの表示
	return result;
}