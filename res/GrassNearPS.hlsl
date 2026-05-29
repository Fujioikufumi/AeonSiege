struct GSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : WORLD_POS;
    float2 TerrainUV : TEXCOORD1;
};

Texture2D BaseColorMap : register(t0);
Texture2D MacroTexture : register(t1);
SamplerState BaseColorSmp : register(s0);

float4 main(GSOutput input) : SV_TARGET0
{
    float4 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord);
    clip(baseColor.a - 0.1f);

    float3 terrainColor = MacroTexture.Sample(BaseColorSmp, input.TerrainUV).rgb;

    float rootFactor = 1.0f - input.TexCoord.y;

    float3 blendedColor = baseColor.rgb * terrainColor * 2.0f;
    
    float3 finalColor = lerp(blendedColor, terrainColor, rootFactor /*float3(1.0f, 1.0f, 1.0f)*/);

    return float4(finalColor, 1.0f);
}