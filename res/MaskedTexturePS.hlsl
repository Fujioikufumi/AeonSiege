struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

Texture2D UITexture : register(t0);
SamplerState UISampler : register(s0);

// b1: Mask params
cbuffer UIMaskBuffer : register(b1)
{
    float4 MaskRectUV;   // (u0, v0, u1, v1)
    float4 Params;       // (progress, feather, mode, reserved)
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = UITexture.Sample(UISampler, input.TexCoord);
    float4 finalColor = texColor * input.Color;

    // 既存UIと同じ：ほぼ透明は描かない
    if (finalColor.a < 0.01f)
        discard;

    float progress = saturate(Params.x);
    float feather = max(0.0f, Params.y);
    int mode = (int) (Params.z + 0.5f);

    float u = input.TexCoord.x;
    float v = input.TexCoord.y;

    // マスク判定値（1=描く, 0=捨てる）
    float mask = 1.0f;

    if (mode == 1)
    {
        // Rect clip in UV
        if (u < MaskRectUV.x || u > MaskRectUV.z || v < MaskRectUV.y || v > MaskRectUV.w)
            mask = 0.0f;
    }
    else if (mode == 2)
    {
        // Horizontal progress (left->right)
        // feather を使うなら境界を少し滑らかにする
        if (feather <= 0.0f)
        {
            if (u > progress)
                mask = 0.0f;
        }
        else
        {
            // u <= progress で1、境界付近は smoothstep で落とす
            mask = 1.0f - smoothstep(progress, progress + feather, u);
        }
    }
    else if (mode == 3)
    {
        // Vertical progress (bottom->top as v increases)
        if (feather <= 0.0f)
        {
            if (v > progress)
                mask = 0.0f;
        }
        else
        {
            mask = 1.0f - smoothstep(progress, progress + feather, v);
        }
    }

    if (mask <= 0.0f)
        discard;

    // feather 使用時はαも少し落とす（境界のギザつき軽減）
    finalColor.a *= mask;

    if (finalColor.a < 0.01f)
        discard;

    return finalColor;
}