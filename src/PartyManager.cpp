#include "PartyManager.h"
#include "GameManager.h"
#include "Scene.h"
#include "Paladin.h"
#include "Player.h"
#include "PartyGrowthCatalog.h"
#include "StatusComponent.h"
#include "HealthComponent.h"
#include "Logger.h"
#include "CombatHud.h"

namespace {
	// メンバーのステータスの更新
	void ApplyStatusTo(GameObject* obj, const StatusData& data, bool healToFull)
	{
		if (obj == nullptr) return;
		
		StatusComponent* status = obj->GetComponent<StatusComponent>();
		HealthComponent* health = obj->GetComponent<HealthComponent>();
		
		if (status == nullptr || health == nullptr) return;
		if (!health->IsAlive()) return;

		const float ratio = healToFull ? 1.0f : health->GetHPRatio();
		status->Setup(data);
		const int maxHp = status->GetMaxHp();
		health->SetMaxHP(maxHp);
		health->SetHP(healToFull ? maxHp : max(1, static_cast<int>(maxHp * ratio)));
	}
}

bool PartyManager::Init()
{
	GameObject::Init();
	PartyGrowth::TryLoadPartyGrowth();
	RefreshAllMemberStats(true);
	return true;
}

void PartyManager::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}

int PartyManager::GetPartyExpNeededForNext() const
{
	return PartyGrowth::GetExpNeededForNextLevel(m_PartyLevel);
}

float PartyManager::GetPartyExpRatio() const
{
	// 次のレベルに必要な経験値を取得
	const int need = GetPartyExpNeededForNext();
	if (need <= 0)
		return 1.0f;
	return std::clamp(static_cast<float>(m_PartyExpTowardNext) / static_cast<float>(need), 0.0f, 1.0f);
}

void PartyManager::ProcessPartyLevelUps()
{
	bool leveled = false;

	for (;;) // ループで複数レベルアップに対応
	{
		const int need = PartyGrowth::GetExpNeededForNextLevel(m_PartyLevel);
		if (need <= 0)
			break;
		if (m_PartyExpTowardNext < need)
			break;

		m_PartyExpTowardNext -= need; // 経験値を減算して次のレベルに進む
		m_PartyLevel++;				  // レベルアップ
		leveled = true;
	}

	if (leveled)
	{
		// メンバーのステータスを更新し体力を全回復させる
		RefreshAllMemberStats(true);
	}
}

void PartyManager::AddExpToAllies(int exp)
{
	if (exp <= 0) return;
	m_PartyExpTowardNext += exp;
	ProcessPartyLevelUps();
}

void PartyManager::RefreshAllMemberStats(bool healToFull)
{
	if (!PartyGrowth::IsLoaded())
		return;

	Scene* scene = GameManager::GetScene();
	if (scene == nullptr) return;

	StatusData data;

	
	// プレイヤーのステータス更新
	if (Player* player = scene->GetGameObjectByName<Player>("Player"))
	{
		if (PartyGrowth::BuildMemberStatus("Player", m_PartyLevel, data))
			ApplyStatusTo(player, data, healToFull);
	}

	// Paladinのステータスを更新
	if (HasAlly(AllyId::Oscar))
	{
		if (Paladin* paladin = scene->GetGameObjectByName<Paladin>("Paladin"))
		{
			if (PartyGrowth::BuildMemberStatus("Paladin", m_PartyLevel, data))
				ApplyStatusTo(paladin, data, healToFull);
		}
	}
}

bool PartyManager::HasAlly(AllyId allyId) const
{
	for (AllyId allies : m_Allies)
	{
		if (allies == allyId)
		{
			return true;
		}
	}
	return false;
}

void PartyManager::AddAlly(AllyId allyId)
{
	if (allyId == AllyId::None || HasAlly(allyId))
	{
		return;
	}

	Scene* scene = GameManager::GetScene();
	if (scene == nullptr)
	{
		return;
	}

	// 加入させる味方のオブジェクトを生成してステータスを更新する
	switch (allyId)
	{
	case AllyId::Oscar:
	{
		Paladin* paladin = scene->AddGameObject<Paladin>(eLayer::ALLY, "Paladin");
		if (paladin != nullptr)
		{
			paladin->SetPosition({ -450.0f, 0.0f, -620.0f });
			paladin->SetIsCulled(true);
			m_Allies.push_back(allyId);
			RefreshAllMemberStats(false);
			if (CombatHud* hud = scene->GetGameObjectByName<CombatHud>("CombatHud"))
				hud->RefreshLayout();
		}
		break;
	}

	default:
		break;
	}
}