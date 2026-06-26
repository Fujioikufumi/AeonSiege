#ifndef BRDF_HLSLI
#define BRDF_HLSLI

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
#ifndef F_PI
#define F_PI        3.14159265358979323f   // 円周率.
#endif//F_PI


//-----------------------------------------------------------------------------
//      Schlickによるフレネル項の近似式.
//-----------------------------------------------------------------------------
float3 SchlickFresnel(float3 specular, float VH)
{
    return specular + (1.0f - specular) * pow((1.0f - VH), 5.0f);
}

//-----------------------------------------------------------------------------
//      GGXによる法線分布関数.
//-----------------------------------------------------------------------------
float D_GGX(float a, float NH)
{
    float a2 = a * a;
    float f = (NH * NH) * (a2 - 1) + 1;
    return a2 / (F_PI * f * f);
}

//-----------------------------------------------------------------------------
//      Height Correlated Smithによる幾何減衰項.
//-----------------------------------------------------------------------------
float G2_Smith(float NL, float NV, float a)
{
    // ゼロ除算を防ぐため、非常に小さい値でクランプ
    const float epsilon = 0.0001f;
    NL = max(NL, epsilon);
    NV = max(NV, epsilon);
    
    float a2 = a * a;
    float NL2 = NL * NL;
    float NV2 = NV * NV;

    float Lambda_V = (-1.0f + sqrt(a2 * (1.0f - NL2) / NL2 + 1.0f)) * 0.5f;
    float Lambda_L = (-1.0f + sqrt(a2 * (1.0f - NV2) / NV2 + 1.0f)) * 0.5f;
    return 1.0f / (1.0f + Lambda_V + Lambda_L);
}

//-----------------------------------------------------------------------------
//      Lambert BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputeLambert(float3 Kd)
{
    return Kd / F_PI;
}

//-----------------------------------------------------------------------------
//      Phong BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputePhong
(
    float3 Ks,
    float shininess,
    float LdotR
)
{
    return Ks * ((shininess + 2.0f) / (2.0f * F_PI)) * pow(LdotR, shininess);
}

//-----------------------------------------------------------------------------
//      GGX BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputeGGX
(
    float3 Ks,
    float roughness,
    float NdotH,
    float NdotV,
    float NdotL
)
{
    // 0で初期化し、有効な場合のみ計算する（NdotL/NdotVが0以下なら0のまま）
	float3 result = float3(0.0f, 0.0f, 0.0f);

	if (NdotL > 0.0f && NdotV > 0.0f)
	{
		float a = roughness * roughness;
		float D = D_GGX(a, NdotH);
		float G = G2_Smith(NdotL, NdotV, a);
		float3 F = SchlickFresnel(Ks, NdotL);

		result = (D * G * F) / (4.0f * NdotV * NdotL);
	}

	return result;
}

//-----------------------------------------------------------------------------
//      ACESフィルミックトーンマップ (Narkowicz近似).
//      HDRリニア色をLDRへ圧縮し, ハイライトの白飛びを自然に抑える.
//-----------------------------------------------------------------------------
float3 ToneMapACES(float3 x)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//-----------------------------------------------------------------------------
//      リニア色をsRGBガンマ空間へエンコードします.
//      非sRGBバックバッファ(R10G10B10A2_UNORM)へ出力するため手動で補正する.
//-----------------------------------------------------------------------------
float3 LinearToSRGB(float3 c)
{
	return sqrt(saturate(c)); // pow(x,1/2.2)のガンマ2.0近似。高速で見た目はほぼ同等
}

//-----------------------------------------------------------------------------
//      HDRリニア色をトーンマップ+ガンマ補正し, 最終出力色を求めます.
//-----------------------------------------------------------------------------
float3 FinishHDR(float3 hdr)
{
	return LinearToSRGB(ToneMapACES(hdr));
}

//-----------------------------------------------------------------------------
//      半球アンビエント.
//      面の向きに応じて空(上)と地面(下)の環境光を補間し, 平面的な見えを防ぐ.
//-----------------------------------------------------------------------------
float3 HemisphereAmbient(float3 N, float3 skyColor, float3 groundColor)
{
	float hemi = saturate(N.y * 0.5f + 0.5f);
	return lerp(groundColor, skyColor, hemi);
}

#endif//BRDF_HLSLI
