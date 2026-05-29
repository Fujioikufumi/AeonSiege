// UI用頂点シェーダー
struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

cbuffer UISpriteBuffer : register(b0)
{
    float4 Position_Padding;   // Position (xy) + Padding (zw)
    float4 Size_Padding;        // Size (xy) + Padding (zw)
    float4 Scale_Padding;       // Scale (xy) + Padding (zw)
    float4 Rotation_Anchor_Padding; // Rotation (x) + Anchor (yz) + Padding (w)
    float4 ScreenSize_Padding;  // ScreenSize (xy) + Padding (zw)
    float4 Color;               // Color (xyzw)
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    // 値を読み取る
    float2 pos = Position_Padding.xy;
    float2 size = Size_Padding.xy;
    float2 scale = Scale_Padding.xy;
    float rotation = Rotation_Anchor_Padding.x;
    float2 anchor = Rotation_Anchor_Padding.yz;
    float2 screenSize = ScreenSize_Padding.xy;
    
    // ローカル座標（-1～1の範囲）を取得
    float2 localPos = input.Position;
    
    // アンカーポイントを考慮したローカル座標の調整
    float2 offset = (localPos - anchor) * size * scale;
    
    // 回転の適用（アンカーポイントを中心に回転）
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float2 rotatedOffset;
    rotatedOffset.x = offset.x * cosR - offset.y * sinR;
    rotatedOffset.y = offset.x * sinR + offset.y * cosR;
    
    // スクリーン座標に変換
    float2 screenPos = pos + rotatedOffset;
    
    // NDC座標に変換（-1～1の範囲）
    // X: 0～ScreenWidth → -1～1
    // Y: 0～ScreenHeight → 1～-1（Y軸反転）
    float2 ndcPos;
    ndcPos.x = (screenPos.x / screenSize.x) * 2.0 - 1.0;
    ndcPos.y = 1.0 - (screenPos.y / screenSize.y) * 2.0;
    
    output.Position = float4(ndcPos.x, ndcPos.y, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;
    output.Color = Color;

    return output;
}