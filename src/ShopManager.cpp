#include "ShopManager.h"
#include "GameManager.h"
#include "Scene.h"
#include "StatusComponent.h"
#include "PartyManager.h"
#include "Player.h"
#include "SkillComponent.h"
#include "SkillCatalog.h"
#include "UpgradeData.h"
#include <array>
#include <cstdlib>

namespace {

	// 強化内容のメタ情報
	struct UpgradeKindMeta
	{
		UpgradeStatType statType;	// 強化するステータスの種類
		const char* name;			// 強化内容の名前
		const wchar_t* textureFile; // 強化内容のアイコンテクスチャファイル名
		float valuePerRarity;		// レアリティ1あたりの強化値の倍率 (例: 0.05f = レアリティ1で5%強化)
	};

	constexpr std::array<UpgradeKindMeta, kUpgradeKindCount> kUpgradeKindMeta = { {
		{ UpgradeStatType::AttackRate,            "AttackUp",           L"Atk.png",       0.15f },
		{ UpgradeStatType::MoveSpeedRate,         "MoveSpeedUp",        L"Spd.png",       0.05f },
		{ UpgradeStatType::DamageTakenRate,       "DamageReduction",    L"Def.png",       0.05f },
		{ UpgradeStatType::SkillPowerRate,        "SkillPowerUp",       L"SkillAtk.png",  0.15f },
		{ UpgradeStatType::SkillCooldownReduction,"SkillCooldownUp",    L"SkillCul.png",  0.05f },
		{ UpgradeStatType::SkillDurationRate,     "SkillDurationUp",    L"SkillTime.png", 0.05f },
		{ UpgradeStatType::CriticalRate,          "CriticalRateUp",     L"CritRate.png",  0.05f },
		{ UpgradeStatType::CriticalDamageRate,     "CriticalDamageUp",  L"CritDmg.png",   0.10f },
		{ UpgradeStatType::EvasionRate,           "EvasionUp",          L"Evasion.png",   0.03f },
		{ UpgradeStatType::SkillChainDamageRate,  "SkillChainDamageUp", L"Combo.png",     0.10f },
		{ UpgradeStatType::SkillRangeRate,        "SkillRangeUp",       L"SkillRange.png",0.08f },
	} };

	// 強化のレアリティに対応するテクスチャパス
	constexpr std::array<const wchar_t*, kUpgradeRarityCount> kRarityTexturePaths = { {
		L"Assets/Texture/Shop/Rarity1.png",
		L"Assets/Texture/Shop/Rarity2.png",
		L"Assets/Texture/Shop/Rarity3.png",
	} };

	// スキルのテクスチャファイル名 (SkillId::PlayerSlash1 から順に対応させる)
	constexpr std::array<const wchar_t*, 6> kSkillTextureFiles = { {
		L"Skill1.png",
		L"Skill2.png",
		L"Skill3.png",
		L"Skill4.png",
		L"Skill5.png",
		L"Skill6.png",
	} };

	const UpgradeKindMeta& GetUpgradeMeta(UpgradeKind kind)
	{
		return kUpgradeKindMeta[static_cast<int>(kind)];
	}

	std::wstring GetSkillTexturePath(SkillId skillId)
	{
		const int index = static_cast<int>(skillId) - static_cast<int>(SkillId::PlayerSlash1);
		if (index < 0 || index >= static_cast<int>(kSkillTextureFiles.size()))
		{
			return L"Assets/Texture/Shop/Upgrade/Skill1.png";
		}

		const std::wstring basePath = L"Assets/Texture/Shop/Upgrade/";
		return basePath + kSkillTextureFiles[index];
	}

	// スキルのレアリティをスキルIDに基づいて決定する関数
	UpgradeRarity GetSkillOfferRarity(SkillId skillId)
	{
		switch (skillId)
		{
		case SkillId::PlayerSlash1:
		case SkillId::PlayerSlash2:
			return UpgradeRarity::Rarity1;

		case SkillId::PlayerSlash3:
		case SkillId::PlayerSlash4:
			return UpgradeRarity::Rarity2;

		case SkillId::PlayerSlash5:
		case SkillId::PlayerSlash6:
			return UpgradeRarity::Rarity3;

		default:
			return UpgradeRarity::Rarity1;
		}
	}

	// スキルのID順に、プレイヤーがまだ持っていない最初のスキルを返す関数
	static SkillId GetNextUnownedSkill(const SkillComponent* skills)
	{
		if (skills == nullptr)
			return SkillId::None;

		static const SkillId kOrder[] =
		{
			SkillId::PlayerSlash2,
			SkillId::PlayerSlash3,
			SkillId::PlayerSlash4,
			SkillId::PlayerSlash5,
			SkillId::PlayerSlash6,
		};

		for (SkillId id : kOrder)
		{
			if (!skills->HasSkill(id))
				return id;
		}
		return SkillId::None;
	}

	struct RarityRate
	{
		int rarity1;
		int rarity2;
		int rarity3;
	};

	constexpr std::array<RarityRate, 4> kRarityRates = { {
		{ 80, 18,  2 }, // phase 0~2
		{ 65, 28,  7 }, // phase 3~5
		{ 50, 35, 15 }, // phase 6~8
		{ 35, 40, 25 }, // phase 9~
	} };

	const RarityRate& GetRarityRate(int phaseNo)
	{
		if (phaseNo >= 50)
		{
			return kRarityRates[3];
		}
		if (phaseNo >= 30)
		{
			return kRarityRates[2];
		}
		if (phaseNo >= 10)
		{
			return kRarityRates[1];
		}
		return kRarityRates[0];
	}
} // namespace

ShopManager::ShopManager()
	: GameObject()
{
	m_SkillOffersPool.reserve(5);
}

bool ShopManager::Init()
{
	GameObject::Init();
	return true;
}

void ShopManager::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}

void ShopManager::SelectOffer(int index)
{
	if (!m_IsOpen)
	{
		return;
	}

	if (index < 0 || index >= static_cast<int>(m_CurrentOffers.size()))
	{
		return;
	}

	ApplyOffer(m_CurrentOffers[index]);
	m_CurrentOffers.clear();
	m_SelectedIndex = 0;
	m_IsOpen = false;
}

void ShopManager::OpenShop(int phaseNo)
{
	if (m_IsOpen)
	{
		return;
	}

	RollOffers(phaseNo);
	m_SelectedIndex = 0;
	m_IsOpen = true;
}

void ShopManager::RollOffers(int phaseNo)
{
	m_CurrentOffers.clear();
	bool skillOfferPlacedThisShop = false;

	// 味方加入オファーの条件を満たしていれば必ず1つ出す
	if (ShouldOfferAlly(phaseNo, AllyId::Oscar))
	{
		m_CurrentOffers.push_back(CreateJoinAllyOffer(AllyId::Oscar));
	}

	// 残りの枠をランダムなオファーで埋める
	while (static_cast<int>(m_CurrentOffers.size()) < kOptionCount)
	{
		m_CurrentOffers.push_back(RollOffer(phaseNo, skillOfferPlacedThisShop));
	}
}

ShopOffer ShopManager::RollOffer(int phaseNo, bool& skillOfferAlreadyPlacedThisShop)
{
	SkillComponent* skills = GetPlayerSkills();

	const int roll = rand() % 100;

	// フェーズ2以降で30%の確率でスキル解除オファーを出す
	const bool skillGateOk = phaseNo >= 2 && roll < 30;

	// スキル解除オファーは1ショップにつき1つまで、かつプレイヤーがまだ持っていないスキルがある場合にのみ出す
	if (skillGateOk && !skillOfferAlreadyPlacedThisShop && skills != nullptr)
	{
		const SkillId nextId = GetNextUnownedSkill(skills);
		if (nextId != SkillId::None)
		{
			skillOfferAlreadyPlacedThisShop = true;
			return CreateUnlockSkillOffer(nextId);
		}
	}

	// アップグレードの種類をランダムに決定
	const UpgradeKind kind = RollUpgradeKind();

	// アップグレードのレアリティをランダムに決定
	const UpgradeRarity rarity = RollRarity(phaseNo);

	return CreateStatusOffer(kind, rarity);
}

void ShopManager::ApplyOffer(const ShopOffer& offer)
{
	switch (offer.type)
	{
	case ShopOfferType::StatusUpgrade:
		ApplyUpgradeToTeam(offer.upgrade);
		break;

	case ShopOfferType::JoinAlly:
	{
		PartyManager* party = GetPartyManager();
		if (party != nullptr)
		{
			party->AddAlly(offer.allyId);
			m_IsAllyOfferShop = true; // 味方加入済み
		}
		break;
	}

	case ShopOfferType::UnlockSkill:
	{
		SkillComponent* skills = GetPlayerSkills();
		if (skills != nullptr)
		{
			skills->AddSkill(SkillCatalog::Create(offer.skillId));
		}
		break;
	}

	default:
		break;
	}
}

bool ShopManager::ShouldOfferAlly(int phaseNo, AllyId allyId) const
{
	// 既に加入している場合は出さない
	PartyManager* party = GetPartyManager();
	if (party != nullptr && party->HasAlly(allyId))
	{
		return false;
	}

	// Oscar の出現条件
	if (allyId == AllyId::Oscar)
	{
		// 初回はフェーズ 1
		if (phaseNo == 1)
		{
			return true;
		}

		if (phaseNo > 1 && phaseNo % 10 == 0)
		{
			return true;
		}
	}

	return false;
}

void ShopManager::ApplyUpgradeToTeam(const UpgradeData& upgrade)
{
	ApplyUpgradeToLayer(eLayer::PLAYER, upgrade);
	ApplyUpgradeToLayer(eLayer::ALLY, upgrade);
}

void ShopManager::ApplyUpgradeToLayer(eLayer layer, const UpgradeData& upgrade)
{
	Scene* pScene = GetScene();
	if (pScene == nullptr) return;

	const auto& objects = pScene->GetGameObjectsByLayer(layer);

	for (const auto& object : objects)
	{
		if (object == nullptr || object->IsDestroyed())
		{
			continue;
		}

		StatusComponent* status = object->GetComponent<StatusComponent>();
		ApplyUpgradeToStatus(status, upgrade);
	}
}

void ShopManager::ApplyUpgradeToStatus(StatusComponent* status, const UpgradeData& upgrade)
{
	if (status == nullptr)
	{
		return;
	}

	switch (upgrade.statType)
	{
	case UpgradeStatType::AttackRate:
		status->AddAttackRate(upgrade.value);
		break;

	case UpgradeStatType::MoveSpeedRate:
		status->AddMoveSpeedRate(upgrade.value);
		break;

	case UpgradeStatType::DamageTakenRate:
		status->AddDamageReductionRate(upgrade.value);
		break;

	case UpgradeStatType::SkillPowerRate:
		status->AddSkillPowerRate(upgrade.value);
		break;

	case UpgradeStatType::SkillCooldownReduction:
		status->AddSkillCooldownReductionRate(upgrade.value);
		break;

	case UpgradeStatType::SkillDurationRate:
		status->AddSkillDurationRate(upgrade.value);
		break;

	case UpgradeStatType::CriticalRate:
		status->AddCriticalRate(upgrade.value);
		break;

	case UpgradeStatType::CriticalDamageRate:
		status->AddCriticalDamageRate(upgrade.value);
		break;

	case UpgradeStatType::EvasionRate:
		status->AddEvasionRate(upgrade.value);
		break;

	case UpgradeStatType::SkillChainDamageRate:
		status->AddSkillChainDamageRate(upgrade.value);
		break;

	case UpgradeStatType::SkillRangeRate:
		status->AddSkillRangeRate(upgrade.value);
		break;

	default:
		break;
	}
}

ShopOffer ShopManager::CreateStatusOffer(UpgradeKind kind, UpgradeRarity rarity) const
{
	// アップグレードデータを作成
	UpgradeData upgrade = CreateUpgradeData(kind, rarity);

	// ショップに表示する情報をセット
	ShopOffer offer;
	offer.type = ShopOfferType::StatusUpgrade;
	offer.upgrade = upgrade;
	offer.rarityTexturePath = upgrade.rarityTexturePath;
	offer.contentTexturePath = upgrade.contentTexturePath;
	return offer;
}

ShopOffer ShopManager::CreateJoinAllyOffer(AllyId allyId) const
{
	ShopOffer offer;
	offer.type = ShopOfferType::JoinAlly;
	offer.allyId = allyId;

	switch (allyId)
	{
	case AllyId::Oscar:
		offer.contentTexturePath = L"Assets/Texture/Shop/OfferAlly_Oscar.png";
		offer.rarityTexturePath = GetRarityTexturePath(UpgradeRarity::Rarity3);
		break;

		// 時間があれば他の味方も追加したい…
	default:
		break;
	}

	return offer;
}

ShopOffer ShopManager::CreateUnlockSkillOffer(SkillId skillId) const
{
	ShopOffer offer;
	offer.type = ShopOfferType::UnlockSkill;
	offer.skillId = skillId;
	offer.contentTexturePath = GetSkillTexturePath(skillId);
	offer.rarityTexturePath = GetRarityTexturePath(GetSkillOfferRarity(skillId));
	return offer;
}

void ShopManager::MoveSelection(int direction)
{
	if (!m_IsOpen || m_CurrentOffers.empty())
	{
		return;
	}

	m_SelectedIndex += direction;

	if (m_SelectedIndex < 0)
	{
		m_SelectedIndex = static_cast<int>(m_CurrentOffers.size()) - 1;
	}
	else if (m_SelectedIndex >= static_cast<int>(m_CurrentOffers.size()))
	{
		m_SelectedIndex = 0;
	}
}

void ShopManager::SetSelectedIndex(int index)
{
	if (!m_IsOpen || m_CurrentOffers.empty())
	{
		return;
	}
	if (index < 0 || index >= static_cast<int>(m_CurrentOffers.size()))
	{
		return;
	}
	m_SelectedIndex = index;
}

void ShopManager::ConfirmSelection()
{
	SelectOffer(m_SelectedIndex);
}

UpgradeData ShopManager::CreateUpgradeData(UpgradeKind kind, UpgradeRarity rarity) const
{
	// UpgradeDataに引数の値をもとに必要な情報をセットして返す
	UpgradeData data;
	data.kind = kind;
	data.rarity = rarity;
	data.statType = GetStatType(kind);
	data.value = GetUpgradeValue(kind, rarity);
	data.name = GetUpgradeName(kind);
	data.rarityTexturePath = GetRarityTexturePath(rarity);
	data.contentTexturePath = GetContentTexturePath(kind);
	data.id = data.name + "_" + std::to_string(static_cast<int>(rarity));

	return data;
}

UpgradeKind ShopManager::RollUpgradeKind() const
{
	const int value = rand() % kUpgradeKindCount;
	return static_cast<UpgradeKind>(value);
}

UpgradeRarity ShopManager::RollRarity(int phaseNo) const
{
	// レアリティごとの出現率をフェーズ番号に応じて取得
	const RarityRate& rate = GetRarityRate(phaseNo);
	const int roll = rand() % 100;

	if (roll < rate.rarity1)
	{
		return UpgradeRarity::Rarity1;
	}

	if (roll < rate.rarity1 + rate.rarity2)
	{
		return UpgradeRarity::Rarity2;
	}

	return UpgradeRarity::Rarity3;
}

float ShopManager::GetUpgradeValue(UpgradeKind kind, UpgradeRarity rarity) const
{
	// アップグレードの種類とレアリティに応じて強化値を計算して返す
	const UpgradeKindMeta& meta = GetUpgradeMeta(kind);
	return meta.valuePerRarity * static_cast<float>(static_cast<int>(rarity)); // 例：0.05 x レアリティ
}

UpgradeStatType ShopManager::GetStatType(UpgradeKind kind) const
{
	return GetUpgradeMeta(kind).statType;
}

std::wstring ShopManager::GetRarityTexturePath(UpgradeRarity rarity) const
{
	const int index = static_cast<int>(rarity) - 1; // UpgradeRarity::Rarity1 が 0 になるように調整
	if (index < 0 || index >= static_cast<int>(kRarityTexturePaths.size()))
	{
		return kRarityTexturePaths[0];
	}
	return kRarityTexturePaths[index];
}

std::wstring ShopManager::GetContentTexturePath(UpgradeKind kind) const
{
	const UpgradeKindMeta& meta = GetUpgradeMeta(kind);
	return L"Assets/Texture/Shop/Upgrade/" + std::wstring(meta.textureFile);
}

Scene* ShopManager::GetScene() const
{
	return GameManager::GetScene();
}

Player* ShopManager::GetPlayer() const
{
	Scene* scene = GetScene();
	if (scene == nullptr)
	{
		return nullptr;
	}
	return scene->GetGameObjectByName<Player>("Player");
}

PartyManager* ShopManager::GetPartyManager() const
{
	Scene* scene = GetScene();
	if (scene == nullptr)
	{
		return nullptr;
	}
	return scene->GetGameObjectByName<PartyManager>("PartyManager");
}

SkillComponent* ShopManager::GetPlayerSkills() const
{
	Player* player = GetPlayer();
	if (player == nullptr)
	{
		return nullptr;
	}
	return player->GetComponent<SkillComponent>();
}

std::string ShopManager::GetUpgradeName(UpgradeKind kind) const
{
	return GetUpgradeMeta(kind).name;
}