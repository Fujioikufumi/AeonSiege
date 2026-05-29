#pragma once
#include <DirectXMath.h>
#include "GameObject.h"
#include "Input.h"

class Camera : public GameObject
{
public:
	Camera();
	~Camera();
	bool Init() override;
	void Update(float deltaTime = 0.0f) override;

	/// 追従ターゲットの設定（プレイヤーの座標など）
	void SetTrackingTarget(const DirectX::XMFLOAT3& target) { m_TrackingTarget = target; m_HasTrackingTarget = true; }

	/// ロックオンターゲットの設定（敵の座標など）
	void SetLookOnTarget(const DirectX::XMFLOAT3& target) { m_LookOnTarget = target; m_HasLookOnTarget = true; }

	/// ロックオンターゲットの解除
	void ClearLookOnTarget() { m_HasLookOnTarget = false; }

	void UpdateGameView(float deltaTime, MouseState mouseState);
	void UpdateDebugView(float deltaTime, MouseState mouseState);

	/// 視錐台との当たり判定。vCenter を中心とする半径 fRadius の球が視錐台と衝突しているかを判定します。
	/// <returns>1: 内包, 0: 外側, -1: 交差</returns>
	[[nodiscard]] int CollisionViewFrus(const DirectX::XMFLOAT3& vCenter, float fRadius) const;

	[[nodiscard]] const DirectX::XMMATRIX& GetView() const { return m_View; }
	[[nodiscard]] const DirectX::XMMATRIX& GetProj() const { return m_Proj; }
	[[nodiscard]] DirectX::XMFLOAT3 GetTarget() const { return m_Target; }
	[[nodiscard]] float GetDistance() const { return m_CameraDistance; }
	[[nodiscard]] float GetFovY() const { return m_FovY; }
	[[nodiscard]] float GetAspect() const { return m_Aspect; }
	[[nodiscard]] float GetFar() const { return m_Far; }
	[[nodiscard]] DirectX::XMFLOAT4 GetFrustumPlanes(int i) const { return m_Frus[i]; }

	void SetTarget(const DirectX::XMFLOAT3& target) { m_Target = target; }
	void SetFovY(float fovY) { m_FovY = fovY; }
	void SetFar(float farClip) { m_Far = farClip; }

	[[nodiscard]] DirectX::XMFLOAT3 GetViewForwardXZ() const;
	[[nodiscard]] DirectX::XMFLOAT3 GetViewRightXZ() const;

	/// ヒットエフェクト用のカメラシェイクを開始
	void StartHitImpactShake(float maxOffset, float durationSec);

	/// 固定視点モード。m_Position と SetTarget で指定した注視点のみでビューを更新する
	void SetFixedViewMode(bool enabled) { m_FixedViewMode = enabled; }
	[[nodiscard]] bool IsFixedViewMode() const { return m_FixedViewMode; }

private:
	void UpdateFixedView();
	void UpdateHitShake(float deltaTime);
	void CreateViewFrustum();
	void UpdateViewFrustum();
private:
	// --- メンバ変数 ---
	DirectX::XMMATRIX m_View = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_Proj = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT3 m_Target  = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 m_Frus[6] = {};

	// 追従・ロックオン
	DirectX::XMFLOAT3 m_TrackingTarget = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_LookOnTarget = { 0.0f, 0.0f, 0.0f };
	bool m_HasTrackingTarget = false;
	bool m_HasLookOnTarget = false;

	// カメラパラメータ
	float m_FovY	= DirectX::XM_PIDIV4;
	float m_Aspect	= 16.0f / 9.0f;
	float m_Near	= 0.1f;
	float m_Far		= 10000.0f;
	float m_RotateY = 0.0f;
	float m_RotateX = 0.0f;

	float m_CameraDistance		= 45.0f; // 注視点からの距離
	float m_TargetHeightOffset = 10.0f; // 注視点の高さ (キャラクターの上半身ぐらい)
	float m_FollowSpeed			= 10.0f; // カメラの追従速度（大きいほど速く追従）
	float m_LookOnTurnSpeed		= 4.0f;	 // ロックオン時のカメラの振り向き速度
	float m_PitchMax			= 35.0f; // カメラの最大見上げ角度
	float m_PitchMin			= -35.0f; // カメラの最大見下げ角度

	// シェイク機能
	struct ShakeState {
		DirectX::XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };
		float time		= 0.0f;
		float duration	= 0.0f;
		float maxMag	= 0.0f;
	} m_Shake;

	// 視点移動など
	float m_RotationSpeed  = 0.01f;
	float m_DebugMoveSpeed = 90.0f;
	int m_FrameCount = 0;
	int m_WaitFrame = 60;
	bool m_FixedViewMode = false;

	static constexpr float kMinHeightOffset = 1.0f; // カメラの高さの最小値（地面すれすれになるのを防ぐ）
};