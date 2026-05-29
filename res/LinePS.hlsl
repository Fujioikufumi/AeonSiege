struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(VSOutput input) : SV_TARGET0
{
    return input.Color;
}