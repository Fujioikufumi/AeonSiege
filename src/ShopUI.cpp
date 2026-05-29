#include "ShopUI.h"
#include "Sprite.h"
#include "ShopManager.h"
#include "GameManager.h"
#include "Scene.h"
#include "NameSpace.h"
#include "Input.h"
#include "CombatHudLayout.h"

namespace
{
	// ショップUIのスプライトの位置とサイズを設定するユーティリティ関数
	void SetShopSprite(Sprite* sprite, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size)
	{
		if (sprite == nullptr) return;

		sprite->SetPosition(position);
		sprite->SetSize(size);
	}
}

bool ShopUI::Init()
{
	GameObject::Init();

	SetupSprites();
	UpdateLayout();
	SetVisible(false);

	return true;
}

void ShopUI::Update(float deltaTime)
{
	Scene* scene = GameManager::GetScene();
	ShopManager* shopManager = (scene != nullptr)
		? scene->GetGameObjectByName<ShopManager>("ShopManager")
		: nullptr;

	if (shopManager == nullptr || !shopManager->IsOpen())
	{	
		// ショップが開いていない場合はUIを非表示にして更新処理をスキップ
		for (std::wstring& p : m_LastOfferContentPaths)
			p.clear();
		SetVisible(false);
		GameObject::Update(deltaTime);
		return;
	}

	// ショップが開いている場合はUIを表示して更新処理を続行
	SetVisible(true);

	// エフェクトのスクロール
	m_EffectScrollX += kEffectScrollSpeed * deltaTime;
	if (m_EffectScrollX >= HudLayoutUtil::ScreenWidthRatio(1.0f))
	{
		m_EffectScrollX -= HudLayoutUtil::ScreenWidthRatio(1.0f);
	}

	UpdateLayout();
	UpdateOptions(shopManager);
	UpdateInput(shopManager);

	GameObject::Update(deltaTime);
}

void ShopUI::SetupSprites()
{
	m_pEffectA = AddComponent<Sprite>();
	m_pEffectA->Init(L"Assets/Texture/Shop/ShopEffect.png");

	m_pEffectB = AddComponent<Sprite>();
	m_pEffectB->Init(L"Assets/Texture/Shop/ShopEffect.png");

	m_pBackground = AddComponent<Sprite>();
	m_pBackground->Init(L"Assets/Texture/Shop/ShopBackGround.png");

	m_pRedPanel = AddComponent<Sprite>();
	m_pRedPanel->Init(L"Assets/Texture/Shop/ShopRedPanel.png");

	SetupRaritySprites();
	SetupContentSprites();
	SetupOfferSprites();
}

void ShopUI::SetupRaritySprites()
{
	// ショップで提示されるカードの数とレアリティの組み合わせでスプライトを初期化
	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
		{
			const UpgradeRarity rarity = static_cast<UpgradeRarity>(rarityIndex + 1);

			m_RaritySprites[cardIndex][rarityIndex] = AddComponent<Sprite>();
			m_RaritySprites[cardIndex][rarityIndex]->Init(GetRarityTexturePath(rarity));
		}
	}
}

void ShopUI::SetupContentSprites()
{
	// ショップで提示されるカードの数と強化内容の種類・レアリティの組み合わせでスプライトを初期化
	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		for (int kindIndex = 0; kindIndex < kUpgradeKindCount; ++kindIndex)
		{
			for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
			{
				const UpgradeKind kind = static_cast<UpgradeKind>(kindIndex);
				const UpgradeRarity rarity = static_cast<UpgradeRarity>(rarityIndex + 1);

				m_ContentSprites[cardIndex][kindIndex][rarityIndex] = AddComponent<Sprite>();
				m_ContentSprites[cardIndex][kindIndex][rarityIndex]->Init(GetContentTexturePath(kind));
			}
		}
	}
}

void ShopUI::SetupOfferSprites()
{
	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		m_AllySprites[cardIndex] = AddComponent<Sprite>();
		m_AllySprites[cardIndex]->Init(L"Assets/Texture/Shop/OfferAlly_Oscar.png");

		m_SkillSprites[cardIndex] = AddComponent<Sprite>();
		m_SkillSprites[cardIndex]->Init(L"Assets/Texture/Tile.png");
	}
}

void ShopUI::UpdateLayout()
{
	const float screenW = HudLayoutUtil::ScreenWidthRatio(1.0f);
	const float screenH = HudLayoutUtil::ScreenHeightRatio(1.0f);

	const DirectX::XMFLOAT2 screenCenter = { screenW * 0.5f, screenH * 0.5f };
	const DirectX::XMFLOAT2 screenSize = { screenW, screenH };
	const DirectX::XMFLOAT2 effectSize = { screenW, screenH };


	SetShopSprite(m_pEffectA, { screenW * 0.5f - m_EffectScrollX, screenH * 0.5f }, screenSize);
	SetShopSprite(m_pEffectB, { screenW * 1.5f - m_EffectScrollX, screenH * 0.5f }, screenSize);

	SetShopSprite(m_pBackground, screenCenter, screenSize);

	const DirectX::XMFLOAT2 panelSize = { screenW * kPanelSizeRatioW, screenH * kPanelSizeRatioH };
	SetShopSprite(m_pRedPanel, screenCenter, panelSize);

	// レアリティカードのサイズ
	const float rarityCardSize = screenH * kCardSizeRatio;

	// 強化内容のカードのサイズ（レアリティカードと同じサイズで重ねて表示）
	const float contentCardSize = rarityCardSize;

	// カードのY座標（全てのカードで共通）
	const float cardY = screenH * kCardYRatio;

	// ショップに表示するカードの座標とサイズを設定
	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		const DirectX::XMFLOAT2 cardPos = { 
			HudLayoutUtil::ScreenWidthRatio(kCardXRatios[cardIndex]), 
			cardY 
		};
		const DirectX::XMFLOAT2 raritySize = { rarityCardSize, rarityCardSize };
		const DirectX::XMFLOAT2 contentSize = { contentCardSize, contentCardSize };

		for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
		{
			SetShopSprite(m_RaritySprites[cardIndex][rarityIndex], cardPos, raritySize);
		}

		for (int kindIndex = 0; kindIndex < kUpgradeKindCount; ++kindIndex)
		{
			for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
			{
				SetShopSprite(m_ContentSprites[cardIndex][kindIndex][rarityIndex], cardPos, contentSize);
			}
		}

		SetShopSprite(m_AllySprites[cardIndex], cardPos, contentSize);
		SetShopSprite(m_SkillSprites[cardIndex], cardPos, contentSize);
	}
}

void ShopUI::UpdateOptions(const ShopManager* shopManager)
{
	if (shopManager == nullptr) return;

	// UIを更新する前に、全てのオプションスプライトを非表示にしてリセット
	HideAllOptionSprites();

	const auto& offers = shopManager->GetCurrentOffers();
	const int selectedIndex = shopManager->GetSelectedIndex();

	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		if (cardIndex >= static_cast<int>(offers.size()))
		{
			continue;
		}

		const ShopOffer& offer = offers[cardIndex];
		const bool selected = (cardIndex == selectedIndex);

		Sprite* raritySprite = GetRaritySprite(cardIndex, GetOfferRarity(offer));
		Sprite* contentSprite = GetOfferContentSprite(cardIndex, offer);

		ApplyOfferContentSprite(cardIndex, offer, contentSprite);

		SetSpriteVisible(raritySprite, true);
		SetSpriteVisible(contentSprite, true);

		ApplyCardVisual(raritySprite, contentSprite, selected);
	}
}

void ShopUI::ApplyOfferContentSprite(int cardIndex, const ShopOffer& offer, Sprite* contentSprite)
{
	const size_t ix = static_cast<size_t>(cardIndex);

	// オファーの種類がスキル解放や味方加入でない場合は、コンテンツスプライトを非表示にしてキャッシュもクリア
	if (offer.type != ShopOfferType::UnlockSkill && offer.type != ShopOfferType::JoinAlly)
	{
		m_LastOfferContentPaths[ix].clear();
		return;
	}

	if (offer.contentTexturePath.empty() || contentSprite == nullptr)
		return;

	if (m_LastOfferContentPaths[ix] == offer.contentTexturePath)
		return;

	contentSprite->Term();
	if (!contentSprite->Init(offer.contentTexturePath))
	{
		m_LastOfferContentPaths[ix].clear();
		return;
	}

	m_LastOfferContentPaths[ix] = offer.contentTexturePath;
}

void ShopUI::SetVisible(bool visible)
{
	const float alpha = visible ? 1.0f : 0.0f;

	auto setAlpha = [alpha](Sprite* sprite)
		{
			if (sprite != nullptr)
			{
				DirectX::XMFLOAT4 color = sprite->GetColor();
				sprite->SetColor(color.x, color.y, color.z, alpha);
			}
		};

	setAlpha(m_pBackground);
	setAlpha(m_pEffectA);
	setAlpha(m_pEffectB);
	setAlpha(m_pRedPanel);

	if (!visible)
	{
		HideAllOptionSprites();
	}
}

void ShopUI::HideAllOptionSprites()
{
	for (int cardIndex = 0; cardIndex < kShopOfferCount; ++cardIndex)
	{
		for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
		{
			SetSpriteVisible(m_RaritySprites[cardIndex][rarityIndex], false);
		}

		for (int kindIndex = 0; kindIndex < kUpgradeKindCount; ++kindIndex)
		{
			for (int rarityIndex = 0; rarityIndex < kUpgradeRarityCount; ++rarityIndex)
			{
				SetSpriteVisible(m_ContentSprites[cardIndex][kindIndex][rarityIndex], false);
			}
		}

		SetSpriteVisible(m_AllySprites[cardIndex], false);
		SetSpriteVisible(m_SkillSprites[cardIndex], false);
	}
}
void ShopUI::SetSpriteVisible(Sprite* sprite, bool visible)
{
	if (sprite == nullptr) return;

	// Colorの設定
	DirectX::XMFLOAT4 color = sprite->GetColor();
	sprite->SetColor(color.x, color.y, color.z, visible ? 1.0f : 0.0f);
}

Sprite* ShopUI::GetRaritySprite(int cardIndex, UpgradeRarity rarity) const
{
	if (cardIndex < 0 || cardIndex >= kShopOfferCount) return nullptr;

	// UpgradeRarity は 1,2,3 なので、インデックスに変換する際は -1 する
	const int rarityIndex = ToRarityIndex(rarity);
	if (rarityIndex < 0 || rarityIndex >= kUpgradeRarityCount) return nullptr;

	return m_RaritySprites[cardIndex][rarityIndex];
}

Sprite* ShopUI::GetContentSprite(int cardIndex, UpgradeKind kind, UpgradeRarity rarity) const
{
	if (cardIndex < 0 || cardIndex >= kShopOfferCount) return nullptr;

	const int kindIndex = ToKindIndex(kind);
	const int rarityIndex = ToRarityIndex(rarity);

	if (kindIndex < 0 || kindIndex >= kUpgradeKindCount) return nullptr;
	if (rarityIndex < 0 || rarityIndex >= kUpgradeRarityCount) return nullptr;

	return m_ContentSprites[cardIndex][kindIndex][rarityIndex];
}

Sprite* ShopUI::GetOfferContentSprite(int cardIndex, const ShopOffer& offer) const
{
	switch (offer.type)
	{
	case ShopOfferType::StatusUpgrade:
		return GetContentSprite(cardIndex, offer.upgrade.kind, offer.upgrade.rarity);

	case ShopOfferType::JoinAlly:
		if (cardIndex >= 0 && cardIndex < kShopOfferCount)
		{
			return m_AllySprites[cardIndex];
		}
		break;

	case ShopOfferType::UnlockSkill:
		if (cardIndex >= 0 && cardIndex < kShopOfferCount)
		{
			return m_SkillSprites[cardIndex];
		}
		break;

	default:
		break;
	}

	return nullptr;
}

UpgradeRarity ShopUI::GetOfferRarity(const ShopOffer& offer) const
{
	if (offer.type == ShopOfferType::StatusUpgrade)
	{
		return offer.upgrade.rarity;
	}
	if (offer.type == ShopOfferType::JoinAlly)
	{
		return UpgradeRarity::Rarity3;
	}
	return UpgradeRarity::Rarity2;
}

int ShopUI::ToRarityIndex(UpgradeRarity rarity) const
{
	// 整合性のため、UpgradeRarity の値は 1,2,3 として定義されている前提で -1 してインデックスに変換する
	return static_cast<int>(rarity) - 1;
}

int ShopUI::ToKindIndex(UpgradeKind kind) const
{
	return static_cast<int>(kind);
}

std::wstring ShopUI::GetRarityTexturePath(UpgradeRarity rarity) const
{
	switch (rarity)
	{
	case UpgradeRarity::Rarity1:
		return L"Assets/Texture/Shop/Rarity1.png";
	case UpgradeRarity::Rarity2:
		return L"Assets/Texture/Shop/Rarity2.png";
	case UpgradeRarity::Rarity3:
	default:
		return L"Assets/Texture/Shop/Rarity3.png";
	}
}

std::wstring ShopUI::GetContentTexturePath(UpgradeKind kind) const
{
	std::wstring basePath = L"Assets/Texture/Shop/Upgrade/";
	switch (kind)
	{
	case UpgradeKind::AttackUp:
		return basePath + L"Atk.png";
	case UpgradeKind::MoveSpeedUp:
		return basePath + L"Spd.png";
	case UpgradeKind::DamageReduction:
		return basePath + L"Def.png";
	case UpgradeKind::SkillPowerUp:
		return basePath + L"SkillAtk.png";
	case UpgradeKind::SkillCooldownUp:
		return basePath + L"SkillCul.png";
	case UpgradeKind::SkillDurationUp:
		return basePath + L"SkillTime.png";
	case UpgradeKind::CriticalRateUp:
		return basePath + L"CritRate.png";
	case UpgradeKind::CriticalDamageUp:
		return basePath + L"CritDmg.png";
	case UpgradeKind::EvasionUp:
		return basePath + L"Evasion.png";
	case UpgradeKind::SkillChainDamageUp:
		return basePath + L"Combo.png";
	case UpgradeKind::SkillRangeUp:
		return basePath + L"SkillRange.png";
	default:
		return basePath + L"Atk.png";
	}
}

void ShopUI::UpdateInput(ShopManager* shopManager)
{
	if (shopManager == nullptr || !shopManager->IsOpen())
	{
		return;
	}
	if (IsKeyTrigger('1'))
	{
		shopManager->SetSelectedIndex(0);
		return;
	}
	if (IsKeyTrigger('2'))
	{
		shopManager->SetSelectedIndex(1);
		return;
	}
	if (IsKeyTrigger('3'))
	{
		shopManager->SetSelectedIndex(2);
		return;
	}
	if (IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A') || IsControllerTrigger(PAD_LEFT))
	{
		shopManager->MoveSelection(-1);
		return;
	}
	if (IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D') || IsControllerTrigger(PAD_RIGHT))
	{
		shopManager->MoveSelection(1);
		return;
	}
	if (IsKeyTrigger(VK_SPACE) || IsKeyTrigger(VK_RETURN) || IsControllerTrigger(PAD_A))
	{
		shopManager->ConfirmSelection();
		return;
	}
}

void ShopUI::ApplyCardVisual(Sprite* raritySprite, Sprite* contentSprite, bool selected)
{
	const float scale = selected ? kSelectedCardScale : kUnselectedCardScale;
	const float alpha = selected ? kSelectedCardAlpha : kUnselectedCardAlpha;

	if (raritySprite != nullptr)
	{
		raritySprite->SetScale(scale, scale);

		DirectX::XMFLOAT4 color = raritySprite->GetColor();
		raritySprite->SetColor(color.x, color.y, color.z, alpha);
	}

	if (contentSprite != nullptr)
	{
		contentSprite->SetScale(scale, scale);

		DirectX::XMFLOAT4 color = contentSprite->GetColor();
		contentSprite->SetColor(color.x, color.y, color.z, alpha);
	}
}
