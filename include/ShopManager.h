#pragma once
#include "GameObject.h"
#include "ShopOffer.h"
#include <string>
#include <vector>

class Scene;
class Player;
class PartyManager;
class SkillComponent;
class StatusComponent;

class ShopManager : public GameObject
{
public:
	ShopManager();
	~ShopManager() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	void OpenShop(int phaseNo);
	void SelectOffer(int index);

	bool IsOpen() const { return m_IsOpen; }
	const std::vector<ShopOffer>& GetCurrentOffers() const { return m_CurrentOffers; }

	// 選択肢の移動
	void MoveSelection(int direction);
	
	// 選択肢のインデックスを直接設定
	void SetSelectedIndex(int index);
	
	// 現在選択されているオファーを適用してショップを閉じる
	void ConfirmSelection();
	
	// 現在選択されているインデックスを取得
	int GetSelectedIndex() const { return m_SelectedIndex; }

private:
	// ショップオファーの生成と適用
	void RollOffers(int phaseNo);
	ShopOffer RollOffer(int phaseNo, bool& skillOfferAlreadyPlacedThisShop);

	// ショップオファーの条件判定
	bool ShouldOfferAlly(int phaseNo, AllyId allyId) const;

	// ショップオファーの適用
	void ApplyOffer(const ShopOffer& offer);

	// アップグレードオファーの適用
	void ApplyUpgradeToTeam(const UpgradeData& upgrade);

	// レイヤー内の全オブジェクトにアップグレードを適用
	void ApplyUpgradeToLayer(eLayer layer, const UpgradeData& upgrade);

	// パーティのステータスにアップグレードを適用
	void ApplyUpgradeToStatus(StatusComponent* status, const UpgradeData& upgrade);

	// オファーの生成
	ShopOffer CreateStatusOffer(UpgradeKind kind, UpgradeRarity rarity) const;
	
	// 味方加入オファーの生成
	ShopOffer CreateJoinAllyOffer(AllyId allyId) const;
	
	// スキル解除オファーの生成
	ShopOffer CreateUnlockSkillOffer(SkillId skillId) const;

	// アップグレードデータの生成
	UpgradeData CreateUpgradeData(UpgradeKind kind, UpgradeRarity rarity) const;
	
	// アップグレードの種類をランダムに決定
	UpgradeKind RollUpgradeKind() const;
	
	// アップグレードのレアリティをランダムに決定
	UpgradeRarity RollRarity(int phaseNo) const;

	// アップグレードの効果値をレアリティに応じて決定
	float GetUpgradeValue(UpgradeKind kind, UpgradeRarity rarity) const;
	
	// アップグレードの効果の対象となるステータスの種類を取得
	UpgradeStatType GetStatType(UpgradeKind kind) const;
	
	// アップグレードの名前を取得
	std::string GetUpgradeName(UpgradeKind kind) const;

	std::wstring GetRarityTexturePath(UpgradeRarity rarity) const;
	std::wstring GetContentTexturePath(UpgradeKind kind) const;

	Scene* GetScene() const;
	Player* GetPlayer() const;
	PartyManager* GetPartyManager() const;

	// プレイヤーの所有しているスキルコンポーネントを取得
	SkillComponent* GetPlayerSkills() const;

private:
	bool m_IsOpen = false;
	bool m_IsAllyOfferShop = false;
	std::vector<ShopOffer> m_CurrentOffers;
	int m_SelectedIndex = 0;

	static constexpr int kOptionCount = 3;

	std::vector<ShopOffer*> m_SkillOffersPool;
};