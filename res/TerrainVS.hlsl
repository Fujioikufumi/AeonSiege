#include "Common.hlsli"

//-----------------------------------------------------------------------------
//      地形用頂点シェーダーのメインエントリーポイント
//-----------------------------------------------------------------------------
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    // 地形は CbTerrainTransform (b0) を使用
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(localPos, TerrainWorld);
    float4 viewPos = mul(worldPos, TerrainView);
    float4 projPos = mul(viewPos, TerrainProj);
    
    output.Position = projPos;
    output.TexCoord = input.TexCoord;
    output.WorldPos = worldPos.xyz;
    
    // 法線・接線のワールド空間変換
    float3 N = normalize(mul(input.Normal, (float3x3) TerrainWorld));
    float3 T = normalize(mul(input.Tangent, (float3x3) TerrainWorld));
    float3 B = normalize(cross(N, T));
    
    output.InvTangentBasis = transpose(float3x3(T, B, N));
    
    return output;
}