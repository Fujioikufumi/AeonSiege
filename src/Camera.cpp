// #include
//-----------------------------------------------------------------------------
#include "Camera.h"
#include "Input.h"
#include "GameManager.h"
#include "Scene.h"
#include "NameSpace.h"
#include "Terrain.h"
#include <random>
#include <algorithm>

using namespace DirectX;

namespace {
	constexpr float kPi = 3.14159265f;
	// 角度を [-π, π] に収める
	float NormalizeAnglePi(float a)
	{
		a = fmodf(a + kPi, 2.0f * kPi);
		if (a < 0.0f)
			a += 2.0f * kPi;
		return a - kPi;
	}
	float DeltaAngle(float fromRad, float toRad)
	{
		return NormalizeAnglePi(toRad - fromRad);
	}
} // namespace

//-----------------------------------------------------------------------------
//		コンストラクタ
//-----------------------------------------------------------------------------
Camera::Camera()
	: m_CameraDistance(30.0f)
	, m_RotationSpeed(0.0025f)
{
	m_FovY = DirectX::XMConvertToRadians(37.5f);
	m_Aspect = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);

	// 視錐台の作成
	CreateViewFrustum();
}

//-----------------------------------------------------------------------------
// 		デストラクタ
//-----------------------------------------------------------------------------
Camera::~Camera()
{
}

//-----------------------------------------------------------------------------
// 		初期化処理
//-----------------------------------------------------------------------------
bool Camera::Init()
{
	Scene* scene = GameManager::GetScene();
	auto player = scene->GetGameObjectByName("Player");
	if(player != nullptr)
	{
		m_Position = player->GetPosition();
	}
	return true;
}

//-----------------------------------------------------------------------------
//		更新処理
//-----------------------------------------------------------------------------
void Camera::Update(float deltaTime)
{
	UpdateHitShake(deltaTime);
	m_FrameCount++;
	MouseState mouseState = GetMouseState();

	// Ctrlキーでデバッグモード切り替え
	if (IsKeyPress(VK_CONTROL) && m_FrameCount > m_WaitFrame)
	{
		g_isDebugMode = !g_isDebugMode;
		LockMouse(!g_isDebugMode); // デバックモード時はロック解除する
		m_FrameCount = 0;
	}

	if (!m_FixedViewMode)
	{
		if (!g_isDebugMode)
		{
			m_Rotation.y += mouseState.deltaX * m_RotationSpeed;
			m_Rotation.y += GetRightStickX() * 2.0f * deltaTime;
		}
		else if (IsMouseButtonPress(1))
		{
			m_Rotation.y += mouseState.deltaX * m_RotationSpeed;
		}
	}
	if (g_isDebugMode)
	{
		UpdateDebugView(deltaTime, mouseState);
	}
	else if (m_FixedViewMode)
	{
		UpdateFixedView();
	}
	else if (m_HasTrackingTarget)
	{
		UpdateGameView(deltaTime, mouseState);
	}
	UpdateViewFrustum();
}

void Camera::UpdateGameView(float deltaTime, MouseState mouseState)
{
	//----------------------------------------------------------------------
	//	ロックオン中: カメラヨーを「プレイヤー→敵」の方位へ寄せる
	//----------------------------------------------------------------------
	if (m_HasLookOnTarget)
	{
		const float dx = m_LookOnTarget.x - m_TrackingTarget.x;
		const float dz = m_LookOnTarget.z - m_TrackingTarget.z;
		const float distSq = dx * dx + dz * dz;

		if (distSq > 1.0f) // ほぼ同一点では atan2 を避ける
		{
			const float targetYaw = std::atan2(dx, dz);
			const float diff = DeltaAngle(m_Rotation.y, targetYaw);

			const float maxStep = m_LookOnTurnSpeed * deltaTime;
			const float step = std::clamp(diff, -maxStep, maxStep);
			m_Rotation.y = NormalizeAnglePi(m_Rotation.y + step);
		}
	}

	//----------------------------------------------------------------------
	//	カメラの回転更新（マウス＆スティック入力）
	//----------------------------------------------------------------------
	const float pitchMinRad = XMConvertToRadians(m_PitchMin);
	const float pitchMaxRad = XMConvertToRadians(m_PitchMax);

	m_Rotation.x += mouseState.deltaY * m_RotationSpeed; // マウス
	m_Rotation.x -= GetRightStickY() * 1.5f * deltaTime; // スティック
	m_Rotation.x = std::clamp(m_Rotation.x, pitchMinRad, pitchMaxRad);

	const float yaw = m_Rotation.y;
	const float pitch = m_Rotation.x;

	// 中心点となる追いかける座標（プレイヤー位置など）
	XMFLOAT3 playerPos = m_TrackingTarget;

	//----------------------------------------------------------------------
	//	理想のカメラ位置（targetCamPos）の計算
	//----------------------------------------------------------------------
	// 注視点はプレイヤーの位置 + 高さオフセット
	XMFLOAT3 lookAtTarget = m_TrackingTarget;
	lookAtTarget.y += m_TargetHeightOffset;

	// 回転行列を作る
	XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);

	// カメラは設定した注視点から「後ろ(負のZ方向)」に Distance 分離れた位置にある
	XMVECTOR localOffset = XMVectorSet(0.0f, 0.0f, -m_CameraDistance, 0.0f);

	// 回転行列を適用してワールド空間でのオフセットを計算
	XMVECTOR worldOffset = XMVector3TransformCoord(localOffset, rotMat);

	XMFLOAT3 targetCamPos;
	XMVECTOR idealPosVec = XMVectorAdd(XMLoadFloat3(&lookAtTarget), worldOffset);
	XMStoreFloat3(&targetCamPos, idealPosVec);

	//----------------------------------------------------------------------
	//	現在のカメラ位置を理想の位置へ滑らかに補間
	//----------------------------------------------------------------------
	const float t = 1.0f - std::exp(-m_FollowSpeed * deltaTime);

	m_Position.x = std::lerp(m_Position.x, targetCamPos.x, t);
	m_Position.y = std::lerp(m_Position.y, targetCamPos.y, t);
	m_Position.z = std::lerp(m_Position.z, targetCamPos.z, t);

	// 地形があるなら一定の高さ以上下げれないようにする
	Scene* scene = GameManager::GetScene();
	Terrain* terrain = (scene != nullptr) ? scene->GetGameObject<Terrain>() : nullptr;
	if (terrain != nullptr)
	{
		// 地形の高さを取得してカメラのY座標を調整
		float terrainHeight = terrain->GetHeightAt(targetCamPos.x, targetCamPos.z);
		if (m_Position.y < terrainHeight + kMinHeightOffset) // カメラが地面に潜らないように最低1.0fの高さを確保
		{
			m_Position.y = terrainHeight + kMinHeightOffset;
		}
	}

	//----------------------------------------------------------------------
	//	ビュー行列と射影行列の更新
	//----------------------------------------------------------------------
	m_Target = lookAtTarget;

	XMFLOAT3 camPosShake = {
		m_Position.x + m_Shake.offset.x,
		m_Position.y + m_Shake.offset.y,
		m_Position.z + m_Shake.offset.z 
	};

	XMVECTOR posVec = XMLoadFloat3(&camPosShake);
	XMVECTOR targetVec = XMLoadFloat3(&m_Target);
	XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_View = XMMatrixLookAtLH(posVec, targetVec, upVec);
	m_Proj = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_Near, m_Far);
}

void Camera::StartHitImpactShake(float maxOffset, float durationSec)
{
	m_Shake.maxMag = maxOffset;
	m_Shake.duration = durationSec;
	m_Shake.time = durationSec;
	// 初期オフセットをゼロに
	m_Shake.offset = { 0.0f, 0.0f, 0.0f };
}

void Camera::UpdateHitShake(float deltaTime)
{
	if (m_Shake.time <= 0.0f) {
		m_Shake.offset = { 0.0f, 0.0f, 0.0f };
		return;
	}

	m_Shake.time -= deltaTime;
	if (m_Shake.time < 0.0f) {
		m_Shake.time = 0.0f;
		m_Shake.offset = { 0.0f, 0.0f, 0.0f };
		return;
	}

	// 残り時間による減衰比率 (1.0 -> 0.0)
	float ratio = m_Shake.time / m_Shake.duration;
	// 指数関数的な減衰（より自然に止まる）
	float currentMag = m_Shake.maxMag * (ratio * ratio);

	// C++11 以降の乱数生成
	static std::mt19937 engine(std::random_device{}());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	m_Shake.offset.x = dist(engine) * currentMag;
	m_Shake.offset.y = dist(engine) * currentMag;
	m_Shake.offset.z = dist(engine) * currentMag; // カメラの奥行きも揺らすかはお好みで
}

void Camera::UpdateFixedView()
{
	XMFLOAT3 camPosShake = {
		m_Position.x + m_Shake.offset.x,
		m_Position.y + m_Shake.offset.y,
		m_Position.z + m_Shake.offset.z };
	const XMVECTOR posVec = XMLoadFloat3(&camPosShake);
	const XMVECTOR targetVec = XMLoadFloat3(&m_Target);
	const XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_View = XMMatrixLookAtLH(posVec, targetVec, upVec);
	m_Proj = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_Near, m_Far);
}


//------------------------------------------------------------------------------
// 		視錐台の作成
//------------------------------------------------------------------------------
void Camera::CreateViewFrustum()
{
	// 0:上, 1:下, 2:左, 3:右, 4:前, 5:後
	float fTan = tanf(m_FovY * 0.5f);
	m_Frus[0] = DirectX::XMFLOAT4(0.0f, -1.0f, fTan, 0.0f);
	m_Frus[1] = DirectX::XMFLOAT4(0.0f, 1.0f, fTan, 0.0f);
	fTan *= m_Aspect;
	m_Frus[2] = DirectX::XMFLOAT4(1.0f, 0.0f, fTan, 0.0f);
	m_Frus[3] = DirectX::XMFLOAT4(-1.0f, 0.0f, fTan, 0.0f);
	m_Frus[4] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, -m_Near);
	m_Frus[5] = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, m_Far);
	// 0 ~ 5を正規化
	for (int i = 0; i < 6; i++)
	{
		DirectX::XMStoreFloat4(&m_Frus[i],
			DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&m_Frus[i])));
	}
}

//------------------------------------------------------------------------------
// 		視錐台の更新
//------------------------------------------------------------------------------
void Camera::UpdateViewFrustum()
{
	CreateViewFrustum();
	DirectX::XMMATRIX mW = DirectX::XMMatrixTranspose(m_View);
	for (int i = 0; i < 6; ++i)
	{
		DirectX::XMStoreFloat4(&m_Frus[i],
			DirectX::XMPlaneTransform(
				DirectX::XMLoadFloat4(&m_Frus[i]), mW));
	}
}

XMFLOAT3 Camera::GetViewForwardXZ() const
{
	const float sy = std::sin(m_Rotation.y);
	const float cy = std::cos(m_Rotation.y);
	return XMFLOAT3(sy, 0.0f, cy);
}
XMFLOAT3 Camera::GetViewRightXZ() const
{
	XMFLOAT3 f = GetViewForwardXZ();
	const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const XMVECTOR fv = XMLoadFloat3(&f);
	XMVECTOR r = XMVector3Cross(up, fv);

	r = XMVector3Normalize(r);

	XMFLOAT3 out{};
	XMStoreFloat3(&out, r);
	return out;
}

//------------------------------------------------------------------------------
// 		視錐台との当たり判定
//------------------------------------------------------------------------------
int Camera::CollisionViewFrus(const DirectX::XMFLOAT3& vCenter, float fRadius) const
{
	bool bHit = false;
	const XMVECTOR center = XMLoadFloat3(&vCenter);

	for (int i = 0; i < 6; ++i)
	{
		const XMVECTOR planeVec = XMLoadFloat4(&m_Frus[i]);
		const XMVECTOR dotVec = XMPlaneDotCoord(planeVec, center);

		float fDot = 0.0f;
		XMStoreFloat(&fDot, dotVec);

		if (fDot < -fRadius)
			return 0;   // 完全に外側
		if (fDot <= fRadius)
			bHit = true;
	}
	return bHit ? -1 : 1;
}

void Camera::UpdateDebugView(float deltaTime, MouseState mouseState)
{
	if (IsMouseButtonPress(1)) // 右クリック（1 = 右ボタン）
	{
		m_Rotation.y += mouseState.deltaX * m_RotationSpeed; // 左右回転（Y軸回転）
		m_Rotation.x += mouseState.deltaY * m_RotationSpeed; // 上下回転（X軸回転、上下を反転）

		// 回転角度の制限（上下の回転を制限）
		auto maxRadX = DirectX::XMConvertToRadians(89.9f);
		auto minRadX = -DirectX::XMConvertToRadians(89.9f);
		if (m_Rotation.x > maxRadX) m_Rotation.x = maxRadX;
		if (m_Rotation.x < minRadX) m_Rotation.x = minRadX;
	}

	if (IsKeyPress(VK_RIGHT))
	{
		m_Rotation.y += 0.3f / 60.0f;
	}

	// WASDキーでカメラ移動（カメラの向きに基づいて移動）
	float moveSpeed = m_DebugMoveSpeed;

	moveSpeed *= deltaTime;

	// カメラの回転から前方ベクトルを計算
	XMVECTOR forward = XMVectorSet(
		sinf(m_Rotation.y) * cosf(m_Rotation.x),
		-sinf(m_Rotation.x),
		cosf(m_Rotation.y) * cosf(m_Rotation.x),
		0.0f
	);
	forward = XMVector3Normalize(forward);

	// 右方向ベクトル（Y軸を上として、右方向を計算）
	XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVec, forward)); // 外積で計算

	// 現在の位置をロード
	XMVECTOR posVec = XMLoadFloat3(&m_Position);

	// WASD移動（視点回転はしない）
	if (IsKeyPress('W') || IsKeyPress(VK_UP))
	{
		XMVECTOR moveVec = XMVectorScale(forward, moveSpeed);
		posVec = XMVectorAdd(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}
	if (IsKeyPress('S') || IsKeyPress(VK_DOWN))
	{
		XMVECTOR moveVec = XMVectorScale(forward, moveSpeed);
		posVec = XMVectorSubtract(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}
	if (IsKeyPress('A') /*|| IsKeyPress(VK_LEFT)*/)
	{
		XMVECTOR moveVec = XMVectorScale(right, moveSpeed);
		posVec = XMVectorSubtract(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}
	if (IsKeyPress('D') /*|| IsKeyPress(VK_RIGHT)*/)
	{
		XMVECTOR moveVec = XMVectorScale(right, moveSpeed);
		posVec = XMVectorAdd(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}

	// Q/Eキーで上下移動
	if (IsKeyPress('Q'))
	{
		XMVECTOR moveVec = XMVectorScale(upVec, moveSpeed);
		posVec = XMVectorAdd(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}
	if (IsKeyPress('E'))
	{
		XMVECTOR moveVec = XMVectorScale(upVec, moveSpeed);
		posVec = XMVectorSubtract(posVec, moveVec);
		XMStoreFloat3(&m_Position, posVec);
	}

	// ターゲットをカメラの向きで計算（カメラ位置 + 前方方向）
	XMVECTOR targetVec = XMVectorAdd(posVec, forward);
	XMStoreFloat3(&m_Target, targetVec);

	// ビュー行列と投影行列を計算
	{
		XMVECTOR posVec = XMLoadFloat3(&m_Position);
		XMVECTOR targetVec = XMLoadFloat3(&m_Target);
		XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		m_View = XMMatrixLookAtLH(posVec, targetVec, upVec);
		m_Proj = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_Near, m_Far);
	}
}