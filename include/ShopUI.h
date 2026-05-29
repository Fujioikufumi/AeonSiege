#pragma once
#include "GameObject.h"
#include "ShopOffer.h"
#include "UpgradeData.h"
#include <array>
#include <string>

class Sprite;
class ShopManager;

class ShopUI : public GameObject
{
public:
	ShopUI() = default;
	~ShopUI() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

private:
	// ショップUIのスプライトを初期化
	void SetupSprites();
	
	// レアリティを表すスプライトを初期化
	void SetupRaritySprites();
	
	// 強化内容を表すスプライトを初期化
	void SetupContentSprites();
	
	// 味方加入やスキル解放のオファー用スプライトを初期化
	void SetupOfferSprites();

	// ショップUIのレイアウトを更新。画面サイズやカードの配置などを調整
	void UpdateLayout();

	// ShopManagerから現在のオファー情報を取得して、UIのスプライトに反映する
	void UpdateOptions(const ShopManager* shopManager);

	/// ShopManager が設定した ShopOffer.contentTexturePath をコンテンツ用スプライトに適用する
	void ApplyOfferContentSprite(int cardIndex, const ShopOffer& offer, Sprite* contentSprite);

	// ショップUI全体の表示・非表示を切り替える
	void SetVisible(bool visible);

	void HideAllOptionSprites();
	void SetSpriteVisible(Sprite* sprite, bool visible);

	Sprite* GetRaritySprite(int cardIndex, UpgradeRarity rarity) const;
	Sprite* GetContentSprite(int cardIndex, UpgradeKind kind, UpgradeRarity rarity) const;
	Sprite* GetOfferContentSprite(int cardIndex, const ShopOffer& offer) const;

	UpgradeRarity GetOfferRarity(const ShopOffer& offer) const;

	int ToRarityIndex(UpgradeRarity rarity) const;
	int ToKindIndex(UpgradeKind kind) const;

	// ShopManagerにも同様の関数がある
	std::wstring GetRarityTexturePath(UpgradeRarity rarity) const;
	std::wstring GetContentTexturePath(UpgradeKind kind) const;

	void UpdateInput(ShopManager* shopManager);
	void ApplyCardVisual(Sprite* raritySprite, Sprite* contentSprite, bool selected);

private:
	Sprite* m_pBackground = nullptr; // ショップの背景
	Sprite* m_pEffectA = nullptr;	 // ショップの背景エフェクトA
	Sprite* m_pEffectB = nullptr;	 // ショップの背景エフェクトB
	Sprite* m_pRedPanel = nullptr;

	// レアリティスプライト
	std::array<std::array<Sprite*, kShopOfferCount>, kUpgradeRarityCount> m_RaritySprites = {};

	// 強化内容スプライト。UpgradeKind と UpgradeRarity の組み合わせで管理
	std::array<std::array<std::array<Sprite*, kShopOfferCount>, kUpgradeKindCount>, kUpgradeRarityCount> m_ContentSprites = {};

	// 味方加入スプライト
	std::array<Sprite*, kShopOfferCount> m_AllySprites = {};
	
	// スキル解放スプライト
	std::array<Sprite*, kShopOfferCount> m_SkillSprites = {};

	// ShopManager が設定した ShopOffer.contentTexturePath をコンテンツ用スプライトに適用するためのパスのキャッシュ
	std::array<std::wstring, kShopOfferCount> m_LastOfferContentPaths{};

	float m_EffectScrollX = 0.0f;
	static constexpr float kEffectScrollSpeed = 80.0f; // エフェクトのスクロール速度
	static constexpr float kSelectedCardScale = 1.08f; // 選択中のカードのスケール
	static constexpr float kUnselectedCardScale = 1.0f;// 選択中のカードのスケール	
	static constexpr float kSelectedCardAlpha = 1.0f;  // 選択中のカードのアルファ値
	static constexpr float kUnselectedCardAlpha = 0.85f; // 選択されていないカードのアルファ値

	static constexpr float kCardYRatio = 0.43f;		// screenH に対するカードのY座標
	static constexpr float kCardSizeRatio = 0.2f;   // screenH に対するカードサイズ
	static constexpr float kCardXRatios[kShopOfferCount] = { 0.20f, 0.50f, 0.80f }; // screenW に対する各カードのX座標
	static constexpr float kPanelSizeRatioW = 0.5f;	// screenW に対するパネルの幅の割合
	static constexpr float kPanelSizeRatioH = 0.5f;	// screenH に対するパネルの高さの割合
};