#include "Common.hlsli"
#include "BRDF.hlsli"

Texture2D BaseColorMap : register(t0);
Texture2D MetallicMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D NormalMap : register(t3);
SamplerState LinearWrapSmp : register(s0);


PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;

    float4 albedo = float4(BaseColor, Alpha);
    if (HasBaseColorMap != 0)
    {
        albedo *= BaseColorMap.Sample(LinearWrapSmp, input.TexCoord);
    }
    
    if (albedo.a < 0.1f)
        discard;

    float metallic = Metallic;
    if (HasMetallicMap != 0)
    {
        metallic *= MetallicMap.Sample(LinearWrapSmp, input.TexCoord).r;
    }

    float roughness = Roughness;
    if (HasRoughnessMap != 0)
    {
        roughness *= RoughnessMap.Sample(LinearWrapSmp, input.TexCoord).r;
    }

    // 法線計算
    float3 N = normalize(input.InvTangentBasis[2]);
    if (HasNormalMap != 0)
    {
        float3 normalSample = NormalMap.Sample(LinearWrapSmp, input.TexCoord).xyz * 2.0f - 1.0f;
        N = normalize(mul(normalSample, input.InvTangentBasis));
    }

    // ライティング計算
    float3 L = normalize(-LightForward);
    float3 V = normalize(CameraPosition - input.WorldPos);
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));

    // PBR
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);
    
    // Diffuse
    float3 diffuse = ComputeLambert(albedo.rgb * (1.0f - metallic));
    
    // Specular
    float3 specular = ComputeGGX(F0, roughness, NdotH, NdotV, NdotL);

    float3 indirect = AmbientColor * AmbientIntensity * albedo.rgb;
    float3 direct = (diffuse + specular) * LightColor * LightIntensity * NdotL;

    output.Color.rgb = direct + indirect;
    output.Color.a = albedo.a;

    return output;
}