#include "Common.hlsli"

//-----------------------------------------------------------------------------
//      頂点シェーダーのメインエントリーポイント
//-----------------------------------------------------------------------------
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 localPos = float4(input.Position, 1.0f);

    // 行列計算（行優先マトリクス×列ベクトルのため mul(vector, matrix) の順序）
    float4 worldPos = mul(localPos, World);
    float4 viewPos = mul(worldPos, View);
    float4 projPos = mul(viewPos, Proj);

    output.Position = projPos;
    output.TexCoord = input.TexCoord;
    output.WorldPos = worldPos.xyz;

    // 法線・接線の計算
    float3 N = normalize(mul(input.Normal, (float3x3) World));
    float3 T = normalize(mul(input.Tangent, (float3x3) World));
    float3 B = normalize(cross(N, T));

    // 接線空間への基底変換行列
    output.InvTangentBasis = transpose(float3x3(T, B, N));
    
    return output;
}