#pragma once
#include "GameObject.h"
#include "ShopOffer.h"
#include <vector>

/// <summary>
/// パーティ全体のレベル、経験値、メンバーのステータス更新を管理するクラス。
/// </summary>
class PartyManager : public GameObject
{
public:
	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	// 味方を追加
	void AddAlly(AllyId allyId);

	// Idで指定された味方が所属しているか(一体しか実装していない)
	[[nodiscard]] bool HasAlly(AllyId allyId) const;

	/// パーティの全メンバーに経験値を加算
	void AddExpToAllies(int exp);

	[[nodiscard]] int   GetPartyLevel() const { return m_PartyLevel; }
	[[nodiscard]] int   GetPartyExpTowardNext() const { return m_PartyExpTowardNext; }
	[[nodiscard]] int   GetPartyExpNeededForNext() const;
	[[nodiscard]] float GetPartyExpRatio() const;

	/// 現在のレベルに基づき、全メンバーのステータスとHPを再計算・適用
	void RefreshAllMemberStats(bool healToFull);

private:
	/// 累積経験値をチェックし、必要に応じてレベルアップ処理を行う
	void ProcessPartyLevelUps();

private:
	std::vector<AllyId> m_Allies;       // 所属メンバーIDリスト
	int m_PartyLevel = 1;				// パーティの共通レベル
	int m_PartyExpTowardNext = 0;      // 次のレベルまでの累積経験値
};