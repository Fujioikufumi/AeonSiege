
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};


Texture2D SkyTexture : register(t0);
SamplerState SkySmp : register(s0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    // V成分を反転して上下を修正
    float2 texCoord = float2(input.TexCoord.x, -input.TexCoord.y);
    return SkyTexture.Sample(SkySmp, texCoord);
}