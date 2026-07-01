#include "Combat/FloatingDamage.h"
#include "Core/GameManager.h"
#include "Core/Scene.h"
#include "Graphics/Sprite/Sprite.h"

namespace
{
constexpr float kMissSpriteWidth  = 96.0f;                    // MISS表示スプライトの幅
constexpr float kMissSpriteHeight = 36.0f;                    // MISS表示スプライトの高さ
constexpr float kScale            = 0.5f;                     // 数字のスケール
constexpr float kFadeStartTime    = 0.5f;                     // フェード開始時間（秒）
constexpr XMFLOAT4 kDefaultColor  = {1.0f, 1.0f, 1.0f, 1.0f}; // デフォルトの色（白）
constexpr XMFLOAT4 kFadeColor     = {1.0f, 1.0f, 1.0f, 0.0f}; // フェードアウト時の色（透明）
} // namespace

static std::wstring GetNumberTexturePath(FloatingDamageType type)
{
	switch (type)
	{
	case FloatingDamageType::EnemyDamage:
		return L"Assets/Texture/Number/EnemyDamage.png";
	case FloatingDamageType::EnemyCritical:
		return L"Assets/Texture/Number/EnemyCritical.png";
	case FloatingDamageType::AllyDamage:
		return L"Assets/Texture/Number/AllyDamage.png";
	case FloatingDamageType::AllyCritical:
		return L"Assets/Texture/Number/AllyCritical.png";
	case FloatingDamageType::ComboDamage:
		return L"Assets/Texture/Number/Combo.png";
	case FloatingDamageType::ComboCritical:
		return L"Assets/Texture/Number/ComboCritical.png";
	default:
		return L"Assets/Texture/Number/Default.png";
	}
}

bool FloatingDamage::Init()
{
	if (!GameObject::Init())
		return false;

	m_NumberUI = GameManager::GetScene()->AddGameObject<NumberUI>(eLayer::UI, "DamageUI");
	if (m_NumberUI != nullptr)
	{
		m_NumberUI->Hide();
	}

	m_MissSprite = AddComponent<Sprite>();
	if (m_MissSprite != nullptr)
	{
		m_MissSprite->Init(L"Assets/Texture/CombatHud/Miss.png");
		m_MissSprite->SetSize(kMissSpriteWidth, kMissSpriteHeight);
		m_MissSprite->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	m_IsActive = false;
	return true;
}

void FloatingDamage::Setup(int damage, float screenX, float screenY, FloatingDamageType type)
{
	m_PosX     = screenX;
	m_PosY     = screenY;
	m_LifeTime = 0.0f;
	m_IsMiss   = (type == FloatingDamageType::Miss);
	m_IsActive = true;

	if (m_NumberUI != nullptr)
	{
		m_NumberUI->Hide();
	}

	if (m_MissSprite != nullptr)
	{
		m_MissSprite->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	if (m_IsMiss)
	{
		SetupMiss(screenX, screenY);
		return;
	}

	SetupNumber(damage, screenX, screenY, type);
}

void FloatingDamage::Update(float deltaTime)
{
	if (!m_IsActive)
	{
		return;
	}

	// 寿命の管理
	m_LifeTime += deltaTime;
	if (m_LifeTime >= kMaxLifeTime)
	{
		Deactivate();
		return;
	}
	m_PosY -= kMoveSpeed * deltaTime;

	// 寿命が近づくと徐々にフェードする
	float alpha               = 1.0f;
	const float fadeStartTime = kMaxLifeTime * kFadeStartTime;
	if (m_LifeTime > fadeStartTime)
	{
		alpha = 1.0f - ((m_LifeTime - fadeStartTime) / (kMaxLifeTime - fadeStartTime));
	}

	if (m_IsMiss)
	{
		// 回避
		if (m_MissSprite != nullptr)
		{
			m_MissSprite->SetPosition(m_PosX, m_PosY);
			m_MissSprite->SetColor(1.0f, 1.0f, 1.0f, alpha);
		}
	}
	else
	{
		// ダメージ数値
		if (m_NumberUI != nullptr)
		{
			m_NumberUI->SetPosition(m_PosX, m_PosY);
			m_NumberUI->SetColor(1.0f, 1.0f, 1.0f, alpha);
		}
	}
	GameObject::Update(deltaTime);
}


void FloatingDamage::Draw(const RenderContext& context)
{
	if (!m_IsActive)
	{
		return;
	}

	GameObject::Draw(context);
}

void FloatingDamage::Deactivate()
{
	m_IsActive = false;
	m_LifeTime = 0.0f;

	if (m_NumberUI != nullptr)
	{
		m_NumberUI->Hide();
	}

	if (m_MissSprite != nullptr)
	{
		m_MissSprite->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

void FloatingDamage::SetupMiss(float screenX, float screenY)
{
	if (m_MissSprite == nullptr)
	{
		return;
	}

	m_MissSprite->SetPosition(screenX, screenY);
	m_MissSprite->SetSize(kMissSpriteWidth, kMissSpriteHeight);
	m_MissSprite->SetColor(kDefaultColor);
}

void FloatingDamage::SetupNumber(int damage, float screenX, float screenY, FloatingDamageType type)
{
	if (m_NumberUI == nullptr)
	{
		return;
	}

	const std::wstring texturePath = GetNumberTexturePath(type);
	m_NumberUI->SetTexturePath(texturePath);
	m_NumberUI->SetValue(damage);
	m_NumberUI->SetPosition(screenX, screenY);
	m_NumberUI->SetColor(kDefaultColor);
	m_NumberUI->SetScale(kScale);
}
