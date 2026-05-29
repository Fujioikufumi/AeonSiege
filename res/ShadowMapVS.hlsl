#include "Common.hlsli"

// シャドウマップ用の簡略化された出力構造体
struct ShadowVSOutput
{
    float4 Position : SV_POSITION;
};

//-----------------------------------------------------------------------------
//      シャドウマップ生成用頂点シェーダー
//-----------------------------------------------------------------------------
ShadowVSOutput main(VSInput input)
{
    ShadowVSOutput output = (ShadowVSOutput) 0;
    
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(localPos, TerrainWorld);
    float4 viewPos = mul(worldPos, TerrainView);
    float4 projPos = mul(viewPos, TerrainProj);
    
    output.Position = projPos;
    
    return output;
}