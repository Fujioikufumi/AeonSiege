struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

Texture2D UITexture : register(t0);
SamplerState UISampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
     // テクスチャから色を取得
    float4 texColor = UITexture.Sample(UISampler, input.TexCoord);
    
    // 最終的な色とアルファ値を計算
    float4 finalColor = texColor * input.Color;
    
    // アルファテスト：アルファ値が低い（透過部分）は描画しない
    // これにより、透過部分では背景色がそのまま見えるようになる
    //if (finalColor.a < 0.01f)
    //{
    //    discard; // このピクセルを描画しない
    //}
    
    return finalColor;
}