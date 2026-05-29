#pragma once
#include "Component.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include <string>
#include <memory>
#include <DirectXMath.h>

/// <summary>
/// マスク処理や進捗（プログレス）表示が可能な2D UI描画コンポーネント。
/// HPバーやスキルゲージなどに使用されます。
/// </summary>
class MaskedSprite : public Component
{
public:
	// マスクの反映方法
	enum class MaskMode : int
	{
		None = 0,
		RectUV = 1,       // 指定したUV矩形内のみ描画
		Horizontal = 2,   // 水平方向（UV.x <= Progress）で描画
		Vertical = 3,     // 垂直方向（UV.y <= Progress）で描画
	};

private:
	// 頂点シェーダー用定数バッファ (b0)
	struct alignas(16) SpriteBuffer
	{
		DirectX::XMFLOAT4 Position_Padding;
		DirectX::XMFLOAT4 Size_Padding;
		DirectX::XMFLOAT4 Scale_Padding;
		DirectX::XMFLOAT4 Rotation_Anchor_Padding;
		DirectX::XMFLOAT4 ScreenSize_Padding;
		DirectX::XMFLOAT4 Color;
	};

	// ピクセルシェーダー用マスク定数バッファ (b1)
	struct alignas(16) MaskBuffer
	{
		DirectX::XMFLOAT4 MaskRectUV;  // (u0,v0,u1,v1)
		DirectX::XMFLOAT4 Params;      // (progress, feather, mode, unused)
	};

public:
	explicit MaskedSprite(GameObject* obj);
	~MaskedSprite() override;
	bool Init() override;
 	bool Init(const std::wstring& texturePath);
	void Term() override;

protected:
	void Update(float deltaTime) override;
	void Draw(const RenderContext& context) override;

public:
	void SetPosition(float x, float y) { m_Position = { x, y }; }
	[[nodiscard]] DirectX::XMFLOAT2 GetPosition() const { return m_Position; }

	void SetSize(float w, float h) { m_Size = { w, h }; }
	[[nodiscard]] DirectX::XMFLOAT2 GetSize() const { return m_Size; }

	void SetScale(float sx, float sy) { m_Scale = { sx, sy }; }
	[[nodiscard]] DirectX::XMFLOAT2 GetScale() const { return m_Scale; }

	void SetRotation(float rot) { m_Rotation = rot; }
	[[nodiscard]] float GetRotation() const { return m_Rotation; }

	void SetColor(float r, float g, float b, float a = 1.0f) { m_Color = { r, g, b, a }; }
	void SetAlpha(float a) { m_Color.w = a; }
	[[nodiscard]] DirectX::XMFLOAT4 GetColor() const { return m_Color; }

	// 基本となるUV座標の範囲をの設定
	void SetUV(const DirectX::XMFLOAT2& uvMin, const DirectX::XMFLOAT2& uvMax);

	// ゲージの進捗率（0.0～1.0）を設定
	void SetProgress(float progress) { m_Progress = progress; }

	// マスク矩形のUV範囲を直接設定
	void SetMaskRectUV(float u0, float v0, float u1, float v1);

	// マスク境界のフェザー（ぼかし）強度を設定
	void SetFeather(float feather) { m_Feather = feather; }

	// マスクの動作モードを設定
	void SetMaskMode(MaskMode mode) { m_MaskMode = mode; }

private:
	// 描画に必要な頂点バッファを生成
	void CreateVertexBuffer();

	// 各種定数バッファを最新の状態に更新
	void UpdateConstantBuffers();

private:
	std::unique_ptr<Texture> m_Texture;         // テクスチャリソース
	std::wstring m_TexturePath;                 // テクスチャパス
	VertexBuffer m_VertexBuffer;                // 頂点バッファ
	ConstantBuffer m_SpriteCB;                  // トランスフォーム用定数バッファ
	ConstantBuffer m_MaskCB;                    // マスクパラメータ用定数バッファ

	DirectX::XMFLOAT2 m_Position = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 m_Size = { 100.0f, 100.0f };
	DirectX::XMFLOAT2 m_Scale = { 1.0f, 1.0f };
	float             m_Rotation = 0.0f;
	DirectX::XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT2 m_UVMin = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 m_UVMax = { 1.0f, 1.0f };

	MaskMode m_MaskMode = MaskMode::None;
	float    m_Progress = 1.0f;
	float    m_Feather = 0.0f;
	DirectX::XMFLOAT4 m_MaskRectUV = { 0.0f, 0.0f, 1.0f, 1.0f };
};