#include "Common.hlsli"

// 地形生成専用の定数バッファ
cbuffer CbTerrainGen : register(b0)
{
    uint g_Width;
    uint g_Height;
    float g_GridScale;
    float g_HeightScale;
    float g_HeightOffset;
    uint g_CurrentLOD;
    uint g_LODLeft;
    uint g_LODRight;
    uint g_LODTop;
    uint g_LODBottom;
    uint g_HeightMapStartX;
    uint g_HeightMapStartZ;
    uint g_HeightMapChunkPixelsX;
    uint g_HeightMapChunkPixelsZ;
    float2 _pad0;
    float3 g_ChunkWorldPos;
    float _pad1;
}

Texture2D<float> g_HeightMap : register(t0);
RWStructuredBuffer<TerrainVertex> g_OutVertices : register(u0);

#define TERRAIN_HEIGHT_MAP_SIZE 1024u

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_Width || id.y >= g_Height)
        return;

    uint idx = id.y * g_Width + id.x;
    uint lx = id.x, lz = id.y;

    // ハイトマップ・サンプリング座標の計算
    uint heightMapW = g_HeightMapChunkPixelsX;
    uint heightMapH = g_HeightMapChunkPixelsZ;
    
    uint sx = (g_Width == heightMapW + 1) ? min(lx, heightMapW - 1) : (lx * (heightMapW - 1) / (g_Width - 1));
    uint sz = (g_Height == heightMapH + 1) ? min(lz, heightMapH - 1) : (lz * (heightMapH - 1) / (g_Height - 1));

    uint hx = (lx == g_Width - 1u) ? min(g_HeightMapStartX + g_HeightMapChunkPixelsX, TERRAIN_HEIGHT_MAP_SIZE - 1u) : (g_HeightMapStartX + sx);
    uint hz = (lz == g_Height - 1u) ? min(g_HeightMapStartZ + g_HeightMapChunkPixelsZ, TERRAIN_HEIGHT_MAP_SIZE - 1u) : (g_HeightMapStartZ + sz);

    float h = g_HeightMap[uint2(hx, hz)];
    
    // 位置計算
    float3 pos;
    pos.x = g_ChunkWorldPos.x + (float) lx * g_GridScale;
    pos.y = g_ChunkWorldPos.y + (h - g_HeightOffset) * g_HeightScale;
    pos.z = g_ChunkWorldPos.z + (float) lz * g_GridScale;

    // LOD境界のスカート処理 (隙間埋め)
    if (g_CurrentLOD > 0)
    {
        float skirtDepth = g_CurrentLOD * 10.0f;
        if (lx == 0 || lx == g_Width - 1 || lz == 0 || lz == g_Height - 1)
            pos.y -= skirtDepth;
    }

    // 法線・接線の計算
    float hL = g_HeightMap[uint2(max(0, (int) hx - 1), hz)];
    float hR = g_HeightMap[uint2(min(TERRAIN_HEIGHT_MAP_SIZE - 1, hx + 1), hz)];
    float hU = g_HeightMap[uint2(hx, max(0, (int) hz - 1))];
    float hD = g_HeightMap[uint2(hx, min(TERRAIN_HEIGHT_MAP_SIZE - 1, hz + 1))];

    float3 edgeX = float3(g_GridScale * 2.0f, (hR - hL) * g_HeightScale, 0.0f);
    float3 edgeZ = float3(0.0f, (hD - hU) * g_HeightScale, g_GridScale * 2.0f);
    
    TerrainVertex v;
    v.Position = pos;
    v.Normal = normalize(cross(edgeZ, edgeX));
    v.Tangent = normalize(edgeX);
    v.TexCoord = float2((float) hx / (TERRAIN_HEIGHT_MAP_SIZE - 1), (float) hz / (TERRAIN_HEIGHT_MAP_SIZE - 1));

    g_OutVertices[idx] = v;
}