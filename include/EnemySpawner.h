#pragma once
#include "GameObject.h"
#include "PhaseData.h"
#include <DirectXMath.h>
#include <string>

/// <summary>
/// フェーズ進行に合わせて敵キャラクターを生成（スポーン）するクラス。
/// </summary>
class EnemySpawner : public GameObject
{
public:
	EnemySpawner();
	~EnemySpawner() override = default;

	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	
	/// 指定されたフェーズデータ（ウェーブ内容）に基づいて、敵をスポーンさせます。
	void SpawnPhase(const PhaseData& phaseData);

private:
	/// 指定された中心座標と半径の範囲内で、ランダムなスポーン座標を生成して返します。
	DirectX::XMFLOAT3 CreateRandomSpawnPosition(const DirectX::XMFLOAT3& center, float radius) const;
	
	DirectX::XMFLOAT3 CreateSpawnPosition(
		const DirectX::XMFLOAT3& center,
		float radius,
		const DirectX::XMFLOAT3& playerPos,
		int spreadIndex,
		int spreadCount) const;

	GameObject* SpawnEnemy(EnemyType type, const DirectX::XMFLOAT3& position, int index, int level, int expReward);

private:
	int m_TotalSpawnedCount = 0; // スポナーが存在する間に生成した敵の総数
};