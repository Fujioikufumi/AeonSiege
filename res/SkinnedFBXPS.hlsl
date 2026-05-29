#include "BRDF.hlsli"

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

cbuffer CbLightBuffer : register(b1)
{
    float3 LightColor;
    float LightIntensity;
    float3 LightForward;
    float3 AmbientColor;
    float AmbientIntensity;
};

cbuffer CbCameraBuffer : register(b2)
{
    float3 CameraPosition;
}

cbuffer CbMaterialBuffer : register(b4)
{
    float3 BaseColor;
    float Alpha;
    float Roughness;
    float Metallic;
    uint HasBaseColorMap;
    uint HasMetallicMap;
    uint HasRoughnessMap;
    uint HasNormalMap;
}

Texture2D BaseColorMap : register(t0);
SamplerState BaseColorSmp : register(s0);

Texture2D NormalMap : register(t3);
SamplerState NormalSmp : register(s3);

PSOutput main(VSOutput input)
{
    PSOutput o = (PSOutput) 0;
    
    // ベースカラー
    float3 base = BaseColor;
    if (HasBaseColorMap != 0)
    {
        base = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb * BaseColor;
    }

    // 法線計算
    float3 T0 = input.InvTangentBasis[0];
    float3 B0 = input.InvTangentBasis[1];
    float3 N0 = input.InvTangentBasis[2];

    float3 N = normalize(N0);

    float3 T = normalize(T0 - N * dot(N, T0));

    float signTB = (dot(cross(N, T), B0) < 0.0f) ? -1.0f : 1.0f;
    float3 B = normalize(cross(N, T)) * signTB;

    float3x3 Basis = float3x3(T, B, N);

    // 法線マップがある場合は法線マップを適用
    if (HasNormalMap != 0)
    {
        float3 nTS = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;

        nTS.y = -nTS.y;

        N = normalize(mul(nTS, Basis));
    }

    // ライティング計算
    float3 L = normalize(-LightForward);
    float NL = saturate(dot(N, L));

    // 単純なLambert拡散反射
    float3 ambient = base * AmbientColor * AmbientIntensity;
    float3 direct = base * LightColor * LightIntensity * NL;

    o.Color = float4(saturate(ambient + direct), 1.0f);
    return o;
}