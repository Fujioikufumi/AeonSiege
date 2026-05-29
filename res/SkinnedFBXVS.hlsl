#define MAX_BONES 128

cbuffer CbTransform : register(b0)
{
    float4x4 View;
    float4x4 Proj;
};

cbuffer CbMesh : register(b1)
{
    float4x4 World;
};

cbuffer CbBonePalette : register(b2)
{
    float4x4 BonePalette[MAX_BONES];
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Tangent : TANGENT;

    uint4 BoneIndex : BONEINDICES;
    float4 BoneWeight : BONEWEIGHTS;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;

    float3 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

VSOutput main(VSInput input)
{
    VSOutput o = (VSOutput) 0;

    float4 p = float4(input.Position, 1.0f);

    float4 skinnedP =
          mul(p, BonePalette[input.BoneIndex.x]) * input.BoneWeight.x
        + mul(p, BonePalette[input.BoneIndex.y]) * input.BoneWeight.y
        + mul(p, BonePalette[input.BoneIndex.z]) * input.BoneWeight.z
        + mul(p, BonePalette[input.BoneIndex.w]) * input.BoneWeight.w;

    // Normal/Tangent は w=0（平行移動を無視）
    float4 n4 = float4(input.Normal, 0.0f);
    float tangentSign = input.Tangent.w;
    float4 t4 = float4(input.Tangent.xyz, 0.0f);

    float3 skinnedN =
          mul(n4, BonePalette[input.BoneIndex.x]).xyz * input.BoneWeight.x
        + mul(n4, BonePalette[input.BoneIndex.y]).xyz * input.BoneWeight.y
        + mul(n4, BonePalette[input.BoneIndex.z]).xyz * input.BoneWeight.z
        + mul(n4, BonePalette[input.BoneIndex.w]).xyz * input.BoneWeight.w;

    float3 skinnedT =
          mul(t4, BonePalette[input.BoneIndex.x]).xyz * input.BoneWeight.x
        + mul(t4, BonePalette[input.BoneIndex.y]).xyz * input.BoneWeight.y
        + mul(t4, BonePalette[input.BoneIndex.z]).xyz * input.BoneWeight.z
        + mul(t4, BonePalette[input.BoneIndex.w]).xyz * input.BoneWeight.w;

    float4 worldP = mul(skinnedP, World);
    o.WorldPos = worldP.xyz;

    // World法線/接線（row-vector）
    float3 N = normalize(mul(skinnedN, (float3x3) World));
    float3 T = normalize(mul(skinnedT, (float3x3) World));
    T = normalize(T - N * dot(N, T));
    float detW = determinant((float3x3) World);
    tangentSign *= (detW < 0.0f) ? -1.0f : 1.0f;
    
    float3 B = normalize(cross(N, T)) * tangentSign;

    // row-vector: tangent-space(n) を world へ → mul(n, InvTangentBasis)
    o.InvTangentBasis = float3x3(T, B, N);

    float4 viewP = mul(worldP, View);
    float4 projP = mul(viewP, Proj);

    o.Position = projP;
    o.TexCoord = input.TexCoord;
    return o;
}