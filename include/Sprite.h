#pragma once
#include "DirectXMath.h"
#include "Texture.h"
#include "Component.h"
#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include <string>
#include <memory>

/// <summary>
/// スプライト描画用定数バッファ構造体
/// </summary>
struct alignas(16) SpriteBuffer
{
    DirectX::XMFLOAT4 Position_Padding;       // Position (xy) + Padding (zw)
    DirectX::XMFLOAT4 Size_Padding;           // Size (xy) + Padding (zw)
    DirectX::XMFLOAT4 Scale_Padding;          // Scale (xy) + Padding (zw)
    DirectX::XMFLOAT4 Rotation_Anchor_Padding; // Rotation (x) + Anchor (yz) + Padding (w)
    DirectX::XMFLOAT4 ScreenSize_Padding;      // ScreenSize (xy) + Padding (zw)
    DirectX::XMFLOAT4 Color;                   // Color (xyzw)
};

/// <summary>
/// 2D画像（テクスチャ）を描画するためのコンポーネント。
/// </summary>
class Sprite : public Component
{
public:
    /// コンストラクタ
    explicit Sprite(GameObject* obj);

    /// デストラクタ
    ~Sprite() override;

    bool Init() override;
    /// 指定されたテクスチャパスを使用してスプライトを初期化します。
    bool Init(const std::wstring& texturePath);

    /// 解放処理
    void Term() override;

protected:
    /// 更新処理
    void Update(float deltaTime) override;

    /// 描画処理
    void Draw(const RenderContext& context) override;
public:
    void SetPosition(const DirectX::XMFLOAT2& position) { m_Position = position; }
    void SetPosition(float x, float y) { m_Position = { x, y }; }
    [[nodiscard]] DirectX::XMFLOAT2 GetPosition() const { return m_Position; }

    void SetSize(const DirectX::XMFLOAT2& size) { m_Size = size; }
    void SetSize(float width, float height) { m_Size = { width, height }; }
    [[nodiscard]] DirectX::XMFLOAT2 GetSize() const { return m_Size; }

    void SetScale(const DirectX::XMFLOAT2& scale) { m_Scale = scale; }
    void SetScale(float scaleX, float scaleY) { m_Scale = { scaleX, scaleY }; }
    [[nodiscard]] DirectX::XMFLOAT2 GetScale() const { return m_Scale; }

    void SetRotation(float rotation) { m_Rotation = rotation; }
    [[nodiscard]] float GetRotation() const { return m_Rotation; }

    /// 使用するUV座標の範囲を設定します。
    void SetUV(const DirectX::XMFLOAT2& uvMin, const DirectX::XMFLOAT2& uvMax);

    [[nodiscard]] DirectX::XMFLOAT2 GetUVMin() const { return m_UVMin; }
    [[nodiscard]] DirectX::XMFLOAT2 GetUVMax() const { return m_UVMax; }

    void SetColor(const DirectX::XMFLOAT4& color) { m_Color = color; }
    void SetColor(float r, float g, float b, float a = 1.0f) { m_Color = { r, g, b, a }; }
    void SetAlpha(float a) { m_Color.w = a; }
    [[nodiscard]] DirectX::XMFLOAT4 GetColor() const { return m_Color; }

    /// 使用しているテクスチャオブジェクトを取得します。
    [[nodiscard]] Texture* GetTexture() const { return m_Texture.get(); }

private:
    /// 定数バッファの内容を現在のパラメータで更新します。
    void UpdateConstantBuffer();

    /// 描画に必要な頂点バッファを生成します。
    void CreateVertexBuffer();
private:
    // 描画リソース
    std::unique_ptr<Texture> m_Texture;         // テクスチャリソース
    std::wstring m_TexturePath;                 // テクスチャパス
    VertexBuffer m_VertexBuffer;                // 頂点バッファ
    ConstantBuffer m_ConstantBuffer;            // スプライト用定数バッファ

    // 配置・変形パラメータ
    DirectX::XMFLOAT2 m_Position = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 m_Size = { 100.0f, 100.0f };
    DirectX::XMFLOAT2 m_Scale = { 1.0f, 1.0f };
    float m_Rotation = 0.0f;

    // 外観パラメータ
    DirectX::XMFLOAT2 m_UVMin = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 m_UVMax = { 1.0f, 1.0f };
    DirectX::XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
};