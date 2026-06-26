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

    // ライティング計算（PBR）
	float3 L = normalize(-LightForward);
	float3 V = normalize(CameraPosition - input.WorldPos);
	float3 H = normalize(L + V);

	float NdotL = saturate(dot(N, L));
	float NdotV = saturate(dot(N, V));
	float NdotH = saturate(dot(N, H));

	float roughness = max(Roughness, 0.08f); // 0だと鏡面が鋭すぎるため下限を設定
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), base, Metallic);

	float3 diffuse = ComputeLambert(base * (1.0f - Metallic));
	float3 specular = ComputeGGX(F0, roughness, NdotH, NdotV, NdotL);

    // 半球アンビエント
	float3 ambient = HemisphereAmbient(N, AmbientColor, AmbientColor * 0.3f) * AmbientIntensity * base;

    // フレネルリムライト：輪郭を淡く光らせ、背景からキャラを分離して見せる
	float rim = pow(1.0f - NdotV, 4.0f);
	float3 rimLight = rim * LightColor * 0.4f * NdotL;

	float3 direct = (diffuse + specular) * LightColor * LightIntensity * NdotL;
	float3 hdr = direct + ambient + rimLight;

	o.Color = float4(FinishHDR(hdr), 1.0f);
	return o;
}