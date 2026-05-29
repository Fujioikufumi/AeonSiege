//=============================================================================
// GPU駆動リアルタイム草生成＆カリング (GrassGeneratorCS.hlsl)
//=============================================================================

struct GrassData
{
	float3 Position;
	float Seed;
	float2 TerrainUV;
};

// C++から受け取るパラメータ
cbuffer GenerationParams : register(b0)
{
	float4x4 ViewProj;
	float3 CameraPos;
	float MaxDistance;
	
	// チャンク情報
	float2 ChunkBasePos; // 現在処理しているチャンクの左下ワールドXZ座標
	float ChunkSize; // チャンクの1辺の長さ
	float _pad;

	// 地形情報 (高さとUV計算用)
	float2 TerrainBasePos; // 地形全体の左下ワールドXZ座標
	float TerrainWidth;
	float TerrainHeight;
	float TerrainBaseY; // 地形の基準Y座標
	float HeightScale;
	float HeightOffset;
	float _pad2;
};

// t0: 高さマップ, t1: 岩場判定用フィールドマップ
Texture2D<float> g_HeightMap : register(t0);
Texture2D<float4> g_FieldMap : register(t1);
SamplerState g_Sampler : register(s0);

// u0: 生き残った草の出力バッファ, u1: ExecuteIndirect引数
RWStructuredBuffer<GrassData> g_VisibleGrass : register(u0);
RWStructuredBuffer<uint> g_DrawArgs : register(u1);

//GPU内で疑似乱数を作るためのハッシュ関数
uint pcg_hash(uint seed)
{
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}
float hash_f01(uint seed)
{
	return float(pcg_hash(seed)) / 4294967296.0;
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// 1スレッド = 1本の草の「候補」
	uint index = DTid.x;
	
	// チャンク座標とインデックスを混ぜて、スレッドごとに完全に固有の乱数シードを作る
	uint seed = pcg_hash(asuint(ChunkBasePos.x) ^ pcg_hash(asuint(ChunkBasePos.y) ^ index));
	
	// 1. チャンク内のローカル座標をランダム生成 (0.0 ～ 1.0)
	float localX = hash_f01(seed);
	float localZ = hash_f01(pcg_hash(seed + 1));
	
	// 2. ワールドXZ座標の計算
	float worldX = ChunkBasePos.x + localX * ChunkSize;
	float worldZ = ChunkBasePos.y + localZ * ChunkSize;
	
	// 距離カリング
	float distSq = (worldX - CameraPos.x) * (worldX - CameraPos.x) + (worldZ - CameraPos.z) * (worldZ - CameraPos.z);
	if (distSq > MaxDistance * MaxDistance)
		return;
	
	float dist = sqrt(distSq);
	
	// 密度を下げ始める距離（例：30mまでは100%生やす）
	float lodStartDist = 30.0f;
	
	// 遠ざかるにつれて 1.0(100%) から 0.05(5%) まで生存率を下げる
	float t = saturate((dist - lodStartDist) / (MaxDistance - lodStartDist));
	float currentDensity = lerp(1.0f, 0.05f, t);

	// この草自身の固定乱数（生存スコア）を生成
	float survivalScore = hash_f01(pcg_hash(seed + 777));
	
	// 生存率の合格ラインに達していなければここで消滅！
	if (survivalScore > currentDensity)
		return;

	// 3. 地形全体のUVを計算
	float u = (worldX - TerrainBasePos.x) / TerrainWidth;
	float v = (worldZ - TerrainBasePos.y) / TerrainHeight;
	if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0)
		return;
	
	// FieldMapから色を取得し、白（岩場）なら草を生やさない
	float3 fieldColor = g_FieldMap.SampleLevel(g_Sampler, float2(u, v), 0).rgb;
	float brightness = (fieldColor.r + fieldColor.g + fieldColor.b) * 0.333f; // 明るさを計算
	if (brightness > 0.5f)
		return; // 岩肌には草を生やさない

	// 4. 高さ(Y)の取得
	float h = g_HeightMap.SampleLevel(g_Sampler, float2(u, v), 0);
	float worldY = TerrainBaseY + (h - HeightOffset) * HeightScale;

	// フラスタム（視界）カリング
	float3 posWS = float3(worldX, worldY, worldZ);
	float4 clipPos = mul(float4(posWS, 1.0f), ViewProj);
	clipPos /= clipPos.w;
	
	float threshold = 1.5f; // 視界外にあるとみなす閾値
	if (clipPos.x < -threshold || clipPos.x > threshold ||
		clipPos.y < -threshold || clipPos.y > threshold ||
		clipPos.z < 0.0f || clipPos.z > 1.0f)
		return; // カメラの視界外にある草は描画しない

	uint visibleIndex;
	InterlockedAdd(g_DrawArgs[0], 1, visibleIndex);

	
	GrassData outGrass;
	outGrass.Position = posWS;
	outGrass.Seed = hash_f01(pcg_hash(seed + 999)); // 風で揺らす用の固有シード
	outGrass.TerrainUV = float2(u, v);
	
	g_VisibleGrass[visibleIndex] = outGrass;
}