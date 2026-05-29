struct VSInput
{
    float3 Position : POSITION;
    float Seed : SEED;
    float2 TerrainUV : TEXCOORD;
};
struct VSOutput
{
    float3 PositionWS : POSITION_WS;
    float Seed : SEED;
    float2 TerrainUV : TEXCOORD;
};
VSOutput main(VSInput input)
{
    VSOutput output;
    output.PositionWS = input.Position;
    output.Seed = input.Seed;
    output.TerrainUV = input.TerrainUV;
    return output;
}