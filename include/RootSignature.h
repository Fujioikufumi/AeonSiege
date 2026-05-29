#pragma once
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <ComPtr.h>
#include <d3d12.h>
#include <vector>
#include <cstdint>

enum ShaderStage
{
    ALL = 0,    // 全ステージ
    VS = 1,     // 頂点シェーダ
    HS = 2,     // ハルシェーダ
    DS = 3,     // ドメインシェーダ
    GS = 4,     // ジオメトリシェーダ
    PS = 5,     // ピクセルシェーダ
};

enum SamplerState
{
    PointWrap,          // ポイントサンプリング - 繰り返し
    PointClamp,         // ポイントサンプリング - クランプ
    LinearWrap,         // トライリニアサンプリング - 繰り返し
    LinearClamp,        // トライリニアサンプリング - クランプ
    AnisotropicWrap,    // 異方性サンプリング - 繰り返し
    AnisotropicClamp,   // 異方性サンプリング - クランプ
    ShadowComparison,   // シャドウ比較サンプリング
};

/// <summary>
/// ルートシグネチャを管理するクラス。
/// ComPtrを保持するため、このクラス自体はコピー可能です。
/// </summary>
class RootSignature
{
public:
    class Desc
    {
    public:
        Desc();
        ~Desc();
        Desc& Begin(int count);
        Desc& SetCBV(ShaderStage stage, int index, uint32_t shaderRegister);
        Desc& SetSRV(ShaderStage stage, int index, uint32_t shaderRegister);
        Desc& SetUAV(ShaderStage stage, int index, uint32_t shaderRegister);
        Desc& SetSmp(ShaderStage stage, int index, uint32_t shaderRegister);
        Desc& AddStaticSmp(ShaderStage stage, uint32_t shaderRegister, SamplerState state);
        Desc& AllowIL();
        Desc& AllowSO();
        Desc& AddShadowSampler(ShaderStage stage, uint32_t shaderRegister);
        Desc& End();

        [[nodiscard]] const D3D12_ROOT_SIGNATURE_DESC* GetDesc() const;

    private:
        std::vector<D3D12_DESCRIPTOR_RANGE>     m_Ranges;
        std::vector<D3D12_STATIC_SAMPLER_DESC>  m_Samplers;
        std::vector<D3D12_ROOT_PARAMETER>       m_Params;
        D3D12_ROOT_SIGNATURE_DESC               m_RootDesc;
        bool                                    m_DenyStage[5];
        uint32_t                                m_Flags;

        void CheckStage(ShaderStage stage);
        void SetParam(ShaderStage stage, int index, uint32_t shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE type);
    };

    RootSignature();
    ~RootSignature();

    bool Init(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC* desc);
    void Term();

    [[nodiscard]] ID3D12RootSignature* GetPtr() const;

private:
    ComPtr<ID3D12RootSignature> m_RootSignature;
};