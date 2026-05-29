#pragma once
#include <DirectXMath.h>

class Camera;
namespace MathUtility
{
	/// <summary>
	/// 2つの3Dポイント間の距離を計算します。
	/// </summary>
	/// <param name="pointA">ポイントA</param>
	/// <param name="pointB">ポイントB</param>
	/// <returns>2つのポイント間の距離</returns>
	float CalculateDistance(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);
	
	/// <summary>
	/// 2つの3Dポイント間の距離が指定した範囲内にあるかどうかを判定します。
	/// </summary>
	/// <param name="pointA">ポイントA</param>
	/// <param name="pointB">ポイントB</param>
	/// <param name="range">範囲</param>
	/// <returns>範囲内にある場合はtrue、それ以外はfalse</returns>
	bool IsInRange(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, float range);

	/// <summary>
	/// 2つの3Dポイント間のXZ平面上の距離を計算します。
	/// </summary>
	/// <param name="posA">ポイントA</param>
	/// <param name="posB">ポイントB</param>
	/// <returns>2つのポイント間のXZ平面上の距離</returns>
	float CalculateDistanceXZ(const DirectX::XMFLOAT3& posA, const DirectX::XMFLOAT3& posB);

	/// <summary>
	/// sourcePosからtargetPosを向くクォータニオンを計算します。
	/// </summary>
	/// <param name="sourcePos">開始位置</param>
	/// <param name="targetPos">目標位置</param>
	/// <returns>目標方向を向くクォータニオン</returns>
	DirectX::XMFLOAT4 LookAt(const DirectX::XMFLOAT3& sourcePos, const DirectX::XMFLOAT3& targetPos);

	/// <summary>
	/// 現在の回転から目標の回転へ滑らかに補間(Slerp)します。
	/// </summary>
	/// <param name="currentQuat">現在のクォータニオン</param>
	/// <param name="targetQuat">目標のクォータニオン</param>
	/// <param name="t">補間係数 (0.0f?1.0f)</param>
	/// <returns>補間後のクォータニオン</returns>
	DirectX::XMFLOAT4 SlerpRotation(const DirectX::XMFLOAT4& currentQuat, const DirectX::XMFLOAT4& targetQuat, float t);

	/// <summary>
	/// 現在のYaw(Y軸回転角度)から目標のYawへ、滑らかに回転させます。
	/// </summary>
	/// <param name="currentYaw">現在のYaw角度 (ラジアン)</param>
	/// <param name="targetYaw">目標のYaw角度 (ラジアン)</param>
	/// <param name="turnSpeed">回転速度</param>
	/// <param name="deltaTime">フレーム時間</param>
	/// <returns>補間された新しいYaw角度 (ラジアン)</returns>
	float SlerpYaw(float currentYaw, float targetYaw, float turnSpeed, float deltaTime);

	/// <summary>
	/// ワールド座標をスクリーン座標に変換します。
	/// </summary>
	/// <param name="worldPos">ワールド座標</param>
	/// <param name="camera">カメラ</param>
	/// <returns>スクリーン座標</returns>
	DirectX::XMFLOAT2 WorldToScreen(DirectX::XMFLOAT3 worldPos, Camera* camera);
}