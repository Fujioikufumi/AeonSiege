#include "MathUtility.h"
#include "Camera.h"
#include <cmath>
#include "NameSpace.h"

namespace MathUtility {
	using namespace DirectX;

	float CalculateDistance(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB)
	{
		float dx = pointB.x - pointA.x;
		float dy = pointB.y - pointA.y;
		float dz = pointB.z - pointA.z;
		return sqrtf(dx * dx + dy * dy + dz * dz);
	}
	bool IsInRange(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, float range)
	{
		return CalculateDistance(pointA, pointB) <= range;
	}
	float CalculateDistanceXZ(const XMFLOAT3& posA, const XMFLOAT3& posB)
	{
		float dx = posB.x - posA.x;
		float dz = posB.z - posA.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	XMFLOAT4 LookAt(const XMFLOAT3& sourcePos, const XMFLOAT3& targetPos)
	{
		float dx = targetPos.x - sourcePos.x;
		float dz = targetPos.z - sourcePos.z;
		float angle = std::atan2(dx, dz);

		XMVECTOR quat = XMQuaternionRotationRollPitchYaw(0.0f, angle, 0.0f);

		XMFLOAT4 result;
		XMStoreFloat4(&result, quat);
		return result;
	}

	XMFLOAT4 SlerpRotation(const XMFLOAT4& currentQuat, const XMFLOAT4& targetQuat, float t)
	{
		XMVECTOR qCurrent = XMLoadFloat4(&currentQuat);
		XMVECTOR qTarget = XMLoadFloat4(&targetQuat);
		XMVECTOR qResult = XMQuaternionSlerp(qCurrent, qTarget, t);

		XMFLOAT4 result;
		XMStoreFloat4(&result, qResult);
		return result;
	}

	float SlerpYaw(float currentYaw, float targetYaw, float turnSpeed, float deltaTime)
	{
		// 角度の差分を計算
		float diff = targetYaw - currentYaw;

		// 差分が -PI から PI の範囲内に収まるように正規化 (最短経路の回転)
		while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
		while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

		// 速度と時間を掛けて、1フレームで回転する量を決定
		// (差分以上の回転はしないように min で制限するか、あるいは単に diffに近づける)
		float step = diff * (turnSpeed * deltaTime);

		// 差分の絶対値がステップサイズより小さい場合は、直接ターゲットの角度にする（オーバーシュート防止）
		if (std::abs(diff) < std::abs(step) * 0.5f) { // 簡易的な微調整
			return targetYaw;
		}

		return currentYaw + step;
	}

	DirectX::XMFLOAT2 WorldToScreen(DirectX::XMFLOAT3 worldPos, Camera* camera)
	{
		XMVECTOR vWorldPos = XMLoadFloat3(&worldPos);
		XMVECTOR vScreenPos = XMVector3Project(
			vWorldPos,
			0.0f, 0.0f,					// ビューポートの左上X, Y
			SCREEN_WIDTH, SCREEN_HEIGHT, // ビューポートの幅, 高さ
			0.0f, 1.0f,					// 深度の最小値, 最大値
			camera->GetProj(),
			camera->GetView(),
			XMMatrixIdentity()			// ワールド行列（すでにワールド座標なので単位行列）
		);
		XMFLOAT3 screenPos;
		XMStoreFloat3(&screenPos, vScreenPos);
		// Z値が1.0を超えている場合、カメラの後ろにいるため描画しないなどの判定も可能
		return XMFLOAT2(screenPos.x, screenPos.y);
	}
}