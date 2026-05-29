#ifndef COMMON_HLSLI
#define COMMON_HLSLI

//-----------------------------------------------------------------------------
//      構造体定義
//-----------------------------------------------------------------------------

// 地形用頂点構造体 (コンピュートシェーダー出力用)
struct TerrainVertex
{
    float3 Position;
    float3 Normal;
    float2 TexCoord;
    float3 Tangent;
};

// 頂点シェーダー入力 (Input Assembler経由)
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 Tangent : TANGENT;
};

// 頂点シェーダー出力 / ピクセルシェーダー入力
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

cbuffer CbShadow : register(b3)
{
    float4x4 LightView : packoffset(c0);
    float4x4 LightProj : packoffset(c4);
    float ShadowBias : packoffset(c8);
    float ShadowMapSize : packoffset(c8.y);
    float PCFKernelSize : packoffset(c8.z);
}

// ピクセルシェーダー出力
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

//-----------------------------------------------------------------------------
//      定数バッファ (b0～b4)
//-----------------------------------------------------------------------------

// Transform (Basic用)
cbuffer CbTransform : register(b0)
{
    float4x4 View : packoffset(c0);
    float4x4 Proj : packoffset(c4);
}
cbuffer CbMesh : register(b1)
{
    float4x4 World : packoffset(c0);
}

// Transform (Terrain用)
cbuffer CbTerrainTransform : register(b0)
{
    float4x4 TerrainWorld : packoffset(c0);
    float4x4 TerrainView : packoffset(c4);
    float4x4 TerrainProj : packoffset(c8);
}

// Light
cbuffer CbLight : register(b1)
{
    float3 LightColor : packoffset(c0);
    float LightIntensity : packoffset(c0.w);
    float3 LightForward : packoffset(c1);
    float LightPadding1 : packoffset(c1.w);
    float3 AmbientColor : packoffset(c2);
    float AmbientIntensity : packoffset(c2.w);
}

// Camera
cbuffer CbCamera : register(b2)
{
    float3 CameraPosition : packoffset(c0);
}

// Material
cbuffer CbMaterial : register(b4)
{
    float3 BaseColor : packoffset(c0);
    float Alpha : packoffset(c0.w);
    float Roughness : packoffset(c1);
    float Metallic : packoffset(c1.y);
    uint HasBaseColorMap : packoffset(c1.z);
    uint HasMetallicMap : packoffset(c1.w);
    uint HasRoughnessMap : packoffset(c2);
    uint HasNormalMap : packoffset(c2.y);
}

#endif // COMMON_HLSLI