#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <d3d12.h>
#include <DirectXMath.h>

#include "GameObject.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "Fence.h"
#include "CommandList.h"
#include "DescriptorPool.h"
#include "ConstantBuffer.h"
#include "Material.h"

/// <summary>
/// 地形テクスチャタイプ
/// </summary>
enum class TerrainTextureType
{
	Grass, // 草地
	Water, // 水面
	Sand,  // 砂地
	Rock,  // 岩場
	Dirt,  // 泥地
	Max,
};

/// <summary>
/// 地形頂点構造体
/// </summary>
struct TerrainVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Tangent;
};

/// <summary>
/// 地形描画用定数バッファ構造体
/// </summary>
struct CbTerrainTransform
{
	DirectX::XMFLOAT4X4 World; // ワールド行列
	DirectX::XMFLOAT4X4 View;  // ビュー行列
	DirectX::XMFLOAT4X4 Proj;  // プロジェクション行列
};

/// <summary>
/// 地形生成用定数バッファ構造体(コンピュートシェーダー用)
/// </summary>
struct alignas(256) CbTerrainGen
{
	uint32_t Width;
	uint32_t Height;
	float    GridScale;
	float    HeightScale;

	float    HeightOffset;
	uint32_t CurrentLOD;
	uint32_t LODLeft;
	uint32_t LODRight;

	uint32_t LODTop;
	uint32_t LODBottom;
	uint32_t HeightMapStartX;
	uint32_t HeightMapStartZ;

	uint32_t HeightMapChunkPixelsX;
	uint32_t HeightMapChunkPixelsZ;
	DirectX::XMFLOAT2 _pad0;

	DirectX::XMFLOAT3 ChunkWorldPos;
	float    _pad1;
};

/// <summary>
/// 1つの地形チャンク（近・中・遠の3LODを持つ）
/// </summary>
struct TerrainChunk
{
	static constexpr int kMaxLodLevels = 3;
	static constexpr uint32_t kInvalidLod = 999;

	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
	DescriptorHandle* m_VertexBufferUAV = nullptr;

	IndexBuffer m_IndexBuffers[kMaxLodLevels];
	uint32_t    m_IndexCounts[kMaxLodLevels] = { 0, 0, 0 };

	DirectX::XMFLOAT3 m_WorldCenter = { 0.0f, 0.0f, 0.0f };

	uint32_t m_TargetLOD = 0;       // カメラ距離から算出された理想のLOD
	uint32_t m_CurrentLOD = kInvalidLod;    // 現在VBに書き込まれているLOD
	uint32_t m_LODLeft = 0;
	uint32_t m_LODRight = 0;

	uint32_t m_LODTop = 0;
	uint32_t m_LODBottom = 0;
};

/// <summary>
/// 地形クラス。ハイトマップからの生成、LOD制御、描画を管理する。
/// </summary>
class Terrain : public GameObject
{
public:
	Terrain();
	~Terrain() override;

	bool Init() override;

	/// <summary>
	/// 地形の初期化。ハイトマップやフィールドマップを指定して生成する。
	/// </summary>
	bool Init(const std::wstring& filePath,
		const std::wstring& fieldMapPath,
		float heightScale = 50.0f,
		float gridScale = 1.0f,
		DirectX::XMFLOAT3 centerPos = { 0.0f, 0.0f, 0.0f });

	void Term() override;
	void Update(float deltaTime) override;
	void Draw(const RenderContext& context) override;

	//-----------------------------------------------------------
	// 地形データのロード・設定
	bool LoadMacroTexture(const std::wstring& filePath);
	bool LoadHeightMap(const std::wstring& filePath);
	bool LoadFieldMap(const std::wstring& filePath);
	bool SetTerrainTexture(TerrainTextureType type, const std::wstring& texturePath);

	//-----------------------------------------------------------
	// ゲッター
	[[nodiscard]] float GetHeightAt(float worldX, float worldZ) const;
	[[nodiscard]] std::vector<DirectX::XMFLOAT3> GetFieldMap() const { return m_FieldMap; }
	[[nodiscard]] bool GetFieldMapColor(float worldX, float worldZ, DirectX::XMFLOAT3& outColor) const;
	void GetTerrainWorldBounds(float& outMinX, float& outMaxX, float& outMinZ, float& outMaxZ) const;

	[[nodiscard]] DirectX::XMFLOAT2 GetTerrainVertexCount() const
	{
		return { static_cast<float>(m_Width), static_cast<float>(m_Height) };
	}

	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetMacroTextureHandle() const;
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetHeightMapSRV() const;
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetFieldMapSRV() const;

private:
	//-----------------------------------------------------------
	// 地形生成・リソース作成

	// インデックス作成
	[[nodiscard]] std::vector<uint32_t> CreateIndices(uint32_t gridWidth = 0, uint32_t gridHeight = 0);

	// 
	bool CreateHeightMapTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	bool CreateVertexBufferForCompute(ID3D12Device* device);
	bool ExecuteTerrainVertexGenCompute(ID3D12Device* device, ID3D12CommandQueue* queue, ID3D12GraphicsCommandList* cmdList);
	bool CreateVertexBufferForChunk(ID3D12Device* device, uint32_t gridWidth, uint32_t gridHeight, TerrainChunk* outChunk);
	bool ExecuteTerrainVertexGenComputeChunk(
		ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		uint32_t heightMapStartX, uint32_t heightMapStartZ,
		uint32_t chunkPixelsX, uint32_t chunkPixelsZ,
		uint32_t gridWidth, uint32_t gridHeight,
		const DirectX::XMFLOAT3& chunkWorldOrigin,
		TerrainChunk* chunk,
		uint32_t cbSlotIndex);

private:
	//--- 定数 ---
	static constexpr float kDefaultHeightScale = 1.0f;
	static constexpr float kDefaultGridScale = 1.0f;

	//--- メンバ変数 ---
	// 地形パラメータ
	std::vector<float>    m_HeightMap;			// ハイトマップの高さデータ
	uint32_t              m_Width = 0;			// 地形の横の頂点数 (ハイトマップのX解像度)
	uint32_t              m_Height = 0;			// 地形の奥行きの頂点数 (ハイトマップのY解像度)
	float                 m_HeightScale = kDefaultHeightScale; // ハイトマップのピクセル値を実際の高さに変換するスケール
	float                 m_GridScale = kDefaultGridScale;	// 頂点ごとの距離（グリッドサイズ）
	DirectX::XMFLOAT3     m_CenterPosition = { 0.0f, 0.0f, 0.0f }; // 大本となる地形の中心位置

	// テクスチャ・マップ関連
	std::vector<DirectX::XMFLOAT3> m_FieldMap;
	bool                           m_HasFieldMap = false;
	std::unique_ptr<Texture>       m_FieldMapTexture;
	std::unique_ptr<Texture>       m_MacroTexture;
	std::map<TerrainTextureType, std::unique_ptr<Texture>> m_TerrainTextures;

	// レンダリングリソース
	ConstantBuffer m_TransformCB;
	ConstantBuffer m_MaterialCB;
	Material       m_Material;

	// コンピュートシェーダー(LOD生成)関連
	ComPtr<ID3D12Resource> m_HeightMapTexture;
	ComPtr<ID3D12Resource> m_VertexBufferResource;
	ComPtr<ID3D12Resource> m_HeightMapUploadBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
	DescriptorHandle* m_HeightMapSRV = nullptr;
	DescriptorHandle* m_VertexBufferUAV = nullptr;
	ConstantBuffer    m_TerrainGenCB;

	Fence       m_InitFence;
	CommandList m_InitCommandList;

	std::vector<std::unique_ptr<TerrainChunk>> m_Chunks;
};