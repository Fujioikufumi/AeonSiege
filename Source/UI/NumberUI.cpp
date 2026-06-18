#include "NumberUI.h"
#include "Sprite.h"
#include <cmath>
#include <vector>
#include <algorithm>

bool NumberUI::Init()
{
	if (!GameObject::Init()) return false;

	m_CurrentValue = -1;
	m_PosX = 0.0f;
	m_PosY = 0.0f;
	m_Scale = 1.0f;

	return true;
}

void NumberUI::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}

void NumberUI::SetValue(int value)
{
	// 既に同じ値が設定されている場合は処理をスキップ
	if (m_CurrentValue == value)
		return;

	m_CurrentValue = value;

	int absValue = std::abs(value);

	// 各桁の数字
	std::vector<int> digits;
	if (absValue == 0)
	{
		digits.push_back(0);
	}
	else
	{
		// 数値を10で割りながら各桁を取得
		int temp = absValue;
		while (temp > 0)
		{
			digits.push_back(temp % 10);
			temp /= 10;
		}
	}

	size_t digitCount = digits.size();

	// 必要な桁数の分だけSpriteコンポーネントを追加
	while (m_DigitSprites.size() < digitCount)
	{
		Sprite* sprite = AddComponent<Sprite>();

		// 10分割で数字が並んでいるテクスチャ
		sprite->Init(m_TexturePath);

		m_DigitSprites.push_back(sprite);
	}

	// 各桁の表示を更新（使わない桁は非表示にする）
	for (size_t i = 0; i < m_DigitSprites.size(); ++i)
	{
		Sprite* sprite = m_DigitSprites[i];

		if (i < digitCount)
		{
			int digitValue = digits[i];

			// 11分割のUV計算
			float uvMinX = digitValue * kUVStep;

			// 1文字の幅を足す
			float uvMaxX = uvMinX + kUVStep;

			sprite->SetUV({ uvMinX, 0.0f }, { uvMaxX, 1.0f });

			// 位置とサイズを設定（左から順に並べる）
			float dx = m_PosX - (i * kDigitWidth * m_Scale);
			sprite->SetPosition(dx, m_PosY);
			sprite->SetSize((kDigitWidth * m_Scale) / 2.0f, (kDigitHeight * m_Scale) / 2.0f);

			// 色を反映
			sprite->SetColor(m_Color.x, m_Color.y, m_Color.z, m_Color.w);
		}
		else
		{
			// 余分なSpriteはアルファ0にして画面から消す
			sprite->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
		}
	}
}

void NumberUI::SetPosition(float x, float y)
{
	m_PosX = x;
	m_PosY = y;

	// 位置が変わったら再配置するために強制更新
	int tempValue = m_CurrentValue;
	m_CurrentValue = -1;
	SetValue(tempValue);
}

void NumberUI::SetScale(float scale)
{
	m_Scale = scale;

	// スケールが変わったら再配置するために強制更新
	int tempValue = m_CurrentValue;
	m_CurrentValue = -1;
	SetValue(tempValue);
}

void NumberUI::SetColor(float r, float g, float b, float a)
{
	m_Color.x = r; 
	m_Color.y = g; 
	m_Color.z = b; 
	m_Color.w = a;

	for (size_t i = 0; i < m_DigitSprites.size(); ++i)
	{
		if (m_DigitSprites[i]->GetColor().w > 0.0f) // 現在表示中のものだけ色変更する
		{
			m_DigitSprites[i]->SetColor(r, g, b, a);
		}
	}
}

void NumberUI ::SetColor(const XMFLOAT4& color)
{
	SetColor(color.x, color.y, color.z, color.w);
}

void NumberUI::SetTexturePath(const std::wstring& texturePath)
{
	if (m_TexturePath == texturePath)
	{
		return;
	}

	m_TexturePath = texturePath;

	for (Sprite* sprite : m_DigitSprites)
	{
		if (sprite != nullptr)
		{
			sprite->Term();
			sprite->Init(m_TexturePath);
		}
	}

	m_DigitSprites.clear();

	const int tempValue = m_CurrentValue;
	m_CurrentValue = -1;
	SetValue(tempValue);
}