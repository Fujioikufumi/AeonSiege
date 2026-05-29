cbuffer CbTransform : register(b0)
{
	float4x4 View : packoffset(c0);
	float4x4 Proj : packoffset(c4);
};

cbuffer CbCameraBuffer : register(b2)
{
	float3 CameraPosition : packoffset(c0);
};

// 草を揺らすための風パラメータ
cbuffer CbWind : register(b3)
{
	float g_Time;
	float g_WindDirX;
	float g_WindDirZ;
	float g_Amplitude;
	float g_Frequency;
	float g_Speed;
};

cbuffer CbGenerationParams : register(b0)
{
    float4x4 ViewProj;
    float3 CameraPos;
    float MaxDistance;
    // 以下のパディングはCSとメモリレイアウトを合わせる
    float2 ChunkBasePos;
    float ChunkSize;
    float _pad;
    float2 TerrainBasePos;
    float TerrainWidth;
    float TerrainHeight;
    float TerrainBaseY;
    float HeightScale;
    float HeightOffset;
    float _pad2;
};

// 頂点シェーダーから受け取る情報
struct VSOutput
{
	float3 PositionWS : POSITION_WS;
	float Seed : SEED;
	float2 TerrainUV : TEXCOORD;
};

// ジオメトリシェーダーから次のシェーダー(ピクセルシェーダー)に渡す情報
struct GSOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 WorldPos : WORLD_POS;
	float2 TerrainUV : TEXCOORD1;
};

[maxvertexcount(4)] // 4頂点で1枚の四角形を構成するため、最大4頂点を出力する
void main(point VSOutput input[1], inout TriangleStream<GSOutput> triStream)
{
    float3 basePos = input[0].PositionWS; // 草の根元のワールド座標
    float seed = input[0].Seed;
    float h = 3.0f;
    float w = 1.5f;
    
    float3 toCam = normalize(CameraPos - basePos);
    float3 right = normalize(float3(toCam.z, 0.0f, -toCam.x));
    float3 up = float3(0.0f, 1.0f, 0.0f);
    
    float3 p0 = basePos - right * w;
    float3 p1 = basePos + right * w;
    float3 p2 = basePos - right * (w * 0.35f) + up * h;
    float3 p3 = basePos + right * (w * 0.35f) + up * h;
	
	// 風による揺れの計算
	float2 windXZ = float2(g_WindDirX, g_WindDirZ);
	float lenW = length(windXZ);
	// 風向を正規化（0ベクトルの場合はデフォルト方向を設定）
	float2 windDir = (lenW > 1e-5f) ? (windXZ / lenW) : float2(1.0f, 0.0f);
	
	// sin波による周期的な揺れ
	float phase = seed * 6.2831853f + (basePos.x + basePos.z) * g_Frequency + g_Time * g_Speed;
	float sway = sin(phase) * g_Amplitude;
	float3 bend = float3(windDir.x, 0.0f, windDir.y) * sway;
	// 草の上部を揺らす
    p2 += bend;
    p3 += bend;
    
    GSOutput o;
    o.TerrainUV = input[0].TerrainUV;

    float4 v0 = mul(float4(p0, 1.0f), ViewProj);
    float4 v1 = mul(float4(p1, 1.0f), ViewProj);
    float4 v2 = mul(float4(p2, 1.0f), ViewProj);
    float4 v3 = mul(float4(p3, 1.0f), ViewProj);

	// トライアングルストリップとして4頂点で四角形を構成する
	// 1. 左下 (Bottom Left)
	o.Position = v0;
	o.TexCoord = float2(0, 1);
	o.WorldPos = p0;
	triStream.Append(o);

	// 2. 左上 (Top Left)
	o.Position = v2;
	o.TexCoord = float2(0, 0);
	o.WorldPos = p2;
	triStream.Append(o);

	// 3. 右下 (Bottom Right)
	o.Position = v1;
	o.TexCoord = float2(1, 1);
	o.WorldPos = p1;
	triStream.Append(o);

	// 4. 右上 (Top Right)
	o.Position = v3;
	o.TexCoord = float2(1, 0);
	o.WorldPos = p3;
	triStream.Append(o);

	// トライアングルストリップの終了
	triStream.RestartStrip();
}