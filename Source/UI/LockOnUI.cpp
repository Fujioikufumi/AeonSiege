#include "UI/LockOnUI.h"
#include "Graphics/Sprite/Sprite.h"
#include "Core/GameManager.h"
#include "Core/Scene.h"
#include "Character/Player.h"
#include "Character/Mutant.h"
#include "Input/Camera.h"
#include "Core/NameSpace.h"
#include "Utility/Logger.h"
#include <DirectXMath.h>

// コンポーネント
#include "Combat/HealthComponent.h"
#include "Component/TargetComponent.h"

using namespace DirectX;
namespace {
	constexpr XMFLOAT2 kMarkerSize = {36.0f, 36.0f}; // ロックオンマーカーの一辺サイズ
	constexpr XMFLOAT4 kMarkerColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // ロックオンマーカーの色（RGBA）
	constexpr XMFLOAT4 kMarkerColorTransparent = { 1.0f, 1.0f, 1.0f, 0.0f }; // ロックオンマーカーの透明色

	// ビュー・射影と TextureVS.hlsl の NDC→スクリーン変換に合わせてピクセル座標を求める
	bool WorldToScreenPixel(const XMFLOAT3& world, FXMMATRIX view, CXMMATRIX proj, float* outPx, float* outPy)
	{
		const XMMATRIX vp = XMMatrixMultiply(view, proj);
		const XMVECTOR worldV = XMLoadFloat3(&world);
		const XMVECTOR clipV = XMVector3TransformCoord(worldV, vp);
		const float ndcX = XMVectorGetX(clipV);
		const float ndcY = XMVectorGetY(clipV);
		const float ndcZ = XMVectorGetZ(clipV);
		// クリップ外・カメラ後方は出さない
		if (ndcZ < 0.0f || ndcZ > 1.0f)
			return false;
		const float sw = static_cast<float>(SCREEN_WIDTH);
		const float sh = static_cast<float>(SCREEN_HEIGHT);
		*outPx = (ndcX + 1.0f) * 0.5f * sw;
		*outPy = (1.0f - ndcY) * 0.5f * sh;
		return true;
	}
}

bool LockOnUI::Init()
{
	if (!AddComponent<Sprite>()->Init(L"Assets/Texture/CombatHud/LockOnUI.png"))
	{
		ELOG("Error : Sprite::Init() Failed in LockOnUI.cpp");
		return false;
	}
	Sprite* spr = GetComponent<Sprite>();
	spr->SetSize(kMarkerSize);
	spr->SetColor(1.0f, 1.0f, 1.0f, 0.0f);
	return true;
}

void LockOnUI::Update(float deltaTime)
{
	Sprite* spr = GetComponent<Sprite>();
	if (spr == nullptr)
	{
		GameObject::Update(deltaTime);
		return;
	}
	Scene* scene = GameManager::GetScene();
	Camera* cam = (scene != nullptr) ? scene->GetCamera() : nullptr;
	Player* player = (scene != nullptr) ? scene->GetGameObject<Player>() : nullptr;
	const GameObject* target = (player != nullptr) ? player->GetLockTarget() : nullptr;
	
	if (cam == nullptr || target == nullptr)
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}
	if (scene != nullptr && !scene->ContainsGameObject(target))
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}
	if (target->IsDestroyed())
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}

	HealthComponent* targetHp = (target != nullptr) ? const_cast<GameObject*>(target)->GetComponent<HealthComponent>() : nullptr;
	if (cam == nullptr || target == nullptr || targetHp == nullptr || !targetHp->IsAlive())
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}
	TargetComponent* tComp = const_cast<GameObject*>(target)->GetComponent<TargetComponent>();
	if (tComp == nullptr)
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}
	XMFLOAT3 aim = tComp->GetLockPosition();
	float px = 0.0f;
	float py = 0.0f;
	const XMMATRIX view = cam->GetView();
	const XMMATRIX proj = cam->GetProj();
	if (!WorldToScreenPixel(aim, view, proj, &px, &py))
	{
		spr->SetColor(kMarkerColorTransparent);
		GameObject::Update(deltaTime);
		return;
	}
	spr->SetPosition(px, py);
	spr->SetColor(kMarkerColor);
	GameObject::Update(deltaTime);
}
