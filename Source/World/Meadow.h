#pragma once
#include "GameObject.h"
#include <vector>
#include <DirectXMath.h>
#include <d3d12.h>
#include "RenderContext.h"
#include "ConstantBuffer.h"

/// <summary>
/// GPU-Drivenを用いた大量の草（草原）の描画・生成を管理するクラス。
/// </summary>
class Meadow : public GameObject
{
public:
	/// 草1本あたりのデータ構造
	struct GrassNearData
	{
		DirectX::XMFLOAT3 Position;
		float Seed;
		DirectX::XMFLOAT2 TerrainUV;
	};

	/// コンピュートシェーダー用生成パラメータ
	struct CbGenerationParams
	{
		DirectX::XMFLOAT4X4 ViewProj;
		DirectX::XMFLOAT3 CameraPos;
		float MaxDistance;
		DirectX::XMFLOAT2 ChunkBasePos;
		float ChunkSize;
		float _pad;
		DirectX::XMFLOAT2 TerrainBasePos;
		float TerrainWidth;
		float TerrainHeight;
		float TerrainBaseY;
		float HeightScale;
		float HeightOffset;
		float _pad2;
	};

private:
	struct ChunkInfo { DirectX::XMFLOAT2 BasePos; };

public:
	Meadow();
	~Meadow() override;

	bool Init() override;
	bool Init(class Scene* scene, class Terrain* terrain);
	void Term() override;
	void Update(float deltaTime = 0.0f) override;
	void Draw(const RenderContext& context) override;

private:
	// GPUリソース（バッファ、UAV、シグネチャ）の生成
	bool CreateGPUResources();
private:
	class Scene* m_Scene = nullptr;
	class Terrain* m_Terrain = nullptr;

	// --- 定数バッファ ---
	ConstantBuffer m_CbGrassNear;   // 草揺れアニメーション用
	ConstantBuffer m_CbGeneration;  // 草生成CS用

	// --- GPU-Driven Rendering リソース ---
	ComPtr<ID3D12Resource> m_VisibleGrassBuffer; // 生成された草データ
	ComPtr<ID3D12Resource> m_DrawArgsBuffer;     // 間接描画引数
	ComPtr<ID3D12Resource> m_ArgsResetBuffer;    // 引数リセット用バッファ

	class DescriptorHandle* m_VisibleGrassUAV = nullptr;
	class DescriptorHandle* m_DrawArgsUAV = nullptr;

	ComPtr<ID3D12CommandSignature> m_CommandSignature;

	// --- 管理パラメータ ---
	uint32_t m_MaxGrassCapacity = 2000000;                // 最大草本数
	std::vector<ChunkInfo> m_VisibleChunks;               // 可視チャンクリスト
};