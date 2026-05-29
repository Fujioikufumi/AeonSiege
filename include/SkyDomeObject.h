#pragma once
#include "GameObject.h"
#include "SkyDome.h"
#include "Texture.h"
#include <memory>

/// <summary>
/// スカイドーム（天球）を制御するゲームオブジェクト。
/// </summary>
class SkyDomeObject : public GameObject
{
public:
    SkyDomeObject();
    ~SkyDomeObject() override;
    bool Init() override;
    void Term() override;
    void Update(float deltaTime) override;
    void Draw(const RenderContext& context) override;

    // テクスチャパスを設定します。
    void SetTexturePath(const std::wstring& path) { m_TexturePath = path; }

    // ドームの半径を設定します。
    void SetRadius(float radius) { m_Radius = radius; }

    // スライス数（分割数）を設定します。
    void SetSlices(int slices) { m_Slices = slices; }

    // スタック数（分割数）を設定します。
    void SetStacks(int stacks) { m_Stacks = stacks; }

    // 使用しているテクスチャのGPUハンドルを取得します。
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const;

private:
    SkyDome m_SkyDome;                        // 形状データ
    std::unique_ptr<Texture> m_Texture;       // 天球テクスチャ
    std::wstring m_TexturePath;               // テクスチャファイルのパス

    float m_Radius = 5000.0f;                 // 半径
    int m_Slices = 32;                        // 水平分割数
    int m_Stacks = 32;                        // 垂直分割数
};