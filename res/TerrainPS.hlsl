#include "Common.hlsli"
#include "BRDF.hlsli"

Texture2D FieldMap : register(t4);
Texture2D GrassTexture : register(t5);
Texture2D RockTexture : register(t6);
Texture2D MacroTexture : register(t10);
Texture2D T_MetallicMap : register(t1);
Texture2D T_RoughnessMap : register(t2);
Texture2D T_NormalMap : register(t3);

SamplerState LinearWrapSmp : register(s0);

#define TERRAIN_TILE_SCALE_NEAR   0.05
#define TERRAIN_TILE_SCALE_FAR    0.015
#define TERRAIN_TILE_FAR_START    100.0  
#define TERRAIN_TILE_FAR_RANGE    100.0  

float3 SampleTerrainLayers(float2 uv, float mask)
{
    float3 grass = GrassTexture.Sample(LinearWrapSmp, uv).rgb;
    float3 rock = RockTexture.Sample(LinearWrapSmp, uv).rgb;
    return lerp(grass, rock, mask);
}

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;

    float dist = distance(input.WorldPos, CameraPosition);
  
    float farBlend = saturate((dist - TERRAIN_TILE_FAR_START) / TERRAIN_TILE_FAR_RANGE);

    float2 nearUV = input.WorldPos.xz * TERRAIN_TILE_SCALE_NEAR;
    float2 farUV = input.WorldPos.xz * TERRAIN_TILE_SCALE_FAR;

    // ベースカラー
    float mask = saturate(FieldMap.Sample(LinearWrapSmp, input.TexCoord).r);
    float3 nearColor = SampleTerrainLayers(nearUV, mask);
    
    float3 macroColor = MacroTexture.Sample(LinearWrapSmp, input.TexCoord).rgb;
    float3 farRockDetail = RockTexture.Sample(LinearWrapSmp, farUV).rgb;
    float3 farColor = lerp(macroColor, macroColor * farRockDetail * 1.5f, mask);

    float3 albedo = lerp(nearColor, farColor, farBlend);

    // PBRパラメータ
    float metallic = lerp(Metallic, 0.0f, farBlend);
    float roughness = lerp(Roughness, 0.8f, farBlend);

    // 法線
    float3 N = normalize(input.InvTangentBasis[2]);
    if (HasNormalMap != 0)
    {
        float3 nSample = T_NormalMap.Sample(LinearWrapSmp, input.TexCoord).xyz * 2.0f - 1.0f;
        N = normalize(mul(nSample, input.InvTangentBasis));
    }

    float3 L = normalize(-LightForward);
    float3 V = normalize(CameraPosition - input.WorldPos);
    float NL = saturate(dot(N, L));

    float3 diffuse = ComputeLambert(albedo * (1.0f - metallic));
    float3 ambient = AmbientColor * AmbientIntensity * albedo;
    float3 direct = diffuse * NL * LightColor * LightIntensity;

    output.Color.rgb = saturate(ambient + direct);
    output.Color.rgb *= 2.0f; // 粗さによる減衰
    output.Color.a = Alpha;

    return output;
}