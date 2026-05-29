#pragma once
#include "Component.h"
#include <DirectXMath.h>

class Scene;
class Player;
class Camera;

//-----------------------------------------------------------------------------
// 		味方AIの挙動パラメーター
//-----------------------------------------------------------------------------
struct AllyAIParams
{
	float followStartDistance = 60.0f; // プレイヤーからこの距離以上離れている場合、追従を開始する
	float followStopDistance = 45.0f;  // プレイヤーからこの距離以下になった場合、追従を停止する（待機状態に移行する）

	float waitTargetForwardDistance = 35.0f; // 待機状態の目標位置をプレイヤーの前方にこの距離だけオフセットして設定する
	float waitTargetSideOffset = 20.0f;		 // 待機状態の目標位置をプレイヤーの横方向にこの距離だけオフセットして設定する（正の値で右側、負の値で左側）
	float waitTargetArriveDistance = 3.0f;	 // 待機状態の目標位置にこの距離以内に近づいたら到着と判定する
	float maxWaitTargetDistanceFromPlayer = 60.0f; // 待機状態の目標位置がプレイヤーからこの距離以上離れている場合は、目標位置を再計算する

	float enemyDetectRange = 160.0f; // 敵を索敵する範囲。プレイヤーを中心にこの距離以内にいる敵を攻撃対象として検出する
	float targetKeepRange = 220.0f; // 攻撃対象を維持できる最大距離。攻撃対象がプレイヤーからこの距離以上離れた場合、攻撃対象を見失ったと判定して攻撃対象をクリアする

	float moveSpeed = 45.0f; // 移動速度
	float turnSpeed = 10.0f; // 回転速度

	float playerMoveThreshold = 0.05f; // プレイヤーが動いていると判定する移動量の閾値。プレイヤーの前フレームからの移動距離がこの値以上の場合、プレイヤーは動いていると判定する
};

//-----------------------------------------------------------------------------
// 		味方AIコンポーネント
//-----------------------------------------------------------------------------
class AllyAIComponent : public Component
{
public:
	AllyAIComponent(GameObject* pObj);
	~AllyAIComponent() override = default;

	bool Init() override;
	void Update(float deltaTime) override;

	void Setup(const AllyAIParams& params);

	/// <summary>
	/// プレイヤーを追従するための更新処理。プレイヤーとの距離に応じて追従開始・停止を切り替え、追従中はプレイヤーの位置に向かって移動します。
	/// </summary>
	/// <param name="deltaTime">前フレームとの時間差</param>
	/// <param name="pScene">シーン</param>
	/// <param name="moveSpeed">移動速度</param>
	/// <returns></returns>
	bool UpdateFollowPlayer(float deltaTime, Scene* pScene, float moveSpeed);

	/// <summary>
	/// 指定した目標位置に向かって移動します。目標位置に近づいたら到着と判定して移動を停止します。
	/// </summary>
	/// <param name="deltaTime">前フレームとの時間差</param>
	/// <param name="targetPos">移動の目標位置</param>
	/// <param name="arriveDistance">目標位置に近づいたと判定する距離</param>
	/// <param name="moveSpeed">移動速度</param>
	/// <returns>目標位置に到着したら true、まだ到着していない場合は false</returns>
	bool MoveToTargetPosition(float deltaTime, const DirectX::XMFLOAT3& targetPos, float arriveDistance, float moveSpeed);

	/// <summary>
	/// 指定した目標位置の方向を向くように回転します。回転はターン速度に応じてスムーズに行われます。
	/// </summary>
	/// <param name="deltaTime">前フレームとの時間差</param>
	/// <param name="targetPos">回転の目標位置</param>
	void FaceTarget(float deltaTime, const DirectX::XMFLOAT3& targetPos);

	// プレイヤーを中心に攻撃対象を索敵し、攻撃対象が有効であれば攻撃対象を返します。攻撃対象が無効な場合は nullptr を返します。
	GameObject* GetOrFindAttackTarget(Scene* pScene);

	// 現在の攻撃対象のアドレス
	GameObject* GetAttackTarget() const { return m_AttackTarget; }

	// 攻撃対象をのクリア
	void ClearAttackTarget() { m_AttackTarget = nullptr; }
private:
	// プレイヤーが移動したかどうかの判定
	bool IsPlayerMoving(const DirectX::XMFLOAT3& playerPos);

	// プレイヤーが停止時に自身が待機する座標を作成
	DirectX::XMFLOAT3 CreateWaitTargetPosition(Scene* pScene, Player* pPlayer, Camera* pCamera);
	
	// 待機目標の位置が有効かどうかの判定
	bool IsValidWaitTarget(Camera* pCamera, const DirectX::XMFLOAT3& targetPos, const DirectX::XMFLOAT3& playerPos) const;
	
	// プレイヤーを中心に攻撃対象を索敵し、最も近い敵を返す
	GameObject* FindNearestEnemy(Scene* pScene) const;
	
	// 攻撃対象が有効かどうかの判定
	bool IsValidAttackTarget(GameObject* target) const;
private:
	AllyAIParams m_Params; // 味方AIの挙動パラメーター

	GameObject* m_AttackTarget = nullptr; // 攻撃目標

	DirectX::XMFLOAT3 m_LastPlayerPos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_WaitTargetPos = { 0.0f, 0.0f, 0.0f };

	bool m_HasLastPlayerPos = false;
	bool m_HasWaitTarget = false;
	bool m_IsFollowing = false;
};
