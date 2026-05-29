struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

// 定数バッファ
cbuffer CbLineTransform : register(b0)
{
    float4x4 View : packoffset(c0);
    float4x4 Proj : packoffset(c4);
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 worldPos = float4(input.Position, 1.0f);
    float4 viewPos = mul(worldPos, View);
    float4 clipPos = mul(viewPos, Proj);

    output.Position = clipPos;
    output.Color = input.Color;

    return output;
}