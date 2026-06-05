
#include "Terrain.h"
#include "ResourceManager.h"
#include "Logger.h"
#include <wincodec.h>
#include "LightManager.h"
#include "RenderSystem.h"
#include "ComputeUtil.h"
#include "Scene.h"
#include "GameManager.h"
#include "ImageLoader.h"
#include "Camera.h"

//-----------------------------------------------------------
// 定数
static constexpr uint32_t kHeightMapPixelSize = 1024; // ハイトマップのピクセルサイズ
static constexpr uint32_t kTerrainChunkDiv = 8;		  // 地形を何分割するか
static constexpr uint32_t kTerrainLodLevels = 3;	  // LODレベル数

static constexpr float    kLodDistanceNear = 2400.0f;  // 近景LODの距離閾値
static constexpr float    kLodDistanceMid  = 3600.0f;  // 中景LODの距離閾値

static constexpr float	  kHeightOffset = 1.0f;		  // 高さオフセット


// 各LODの解像度
static constexpr uint32_t kLodSizes[kTerrainLodLevels] = 
{
	kHeightMapPixelSize / kTerrainChunkDiv + 1u, // 近 (+1は重複頂点用)
	kHeightMapPixelSize / kTerrainChunkDiv / 2,  // 中
	kHeightMapPixelSize / kTerrainChunkDiv / 8   // 遠
};

// ルートパラメータのインデックス定義
enum class RootParamIndex : UINT
{
	TransformCB = 0,
	LightCB = 2,
	CameraCB = 3,
	MaterialCB = 4,
	BaseColorMap = 6,
	MetallicMap = 7,
	RoughnessMap = 8,
	NormalMap = 9,
	FieldMap = 10,	  // 地形のどの箇所が岩肌なのかを示すテクスチャ
	GrassTexture = 11,// 近景草テクスチャ
	RockTexture = 12, // 岩肌用のテクスチャ
	MacroTexture = 16 // 遠方の草テクスチャ用に使用
};

//-----------------------------------------------------------------------------
// 		コンストラクタ
//-----------------------------------------------------------------------------
Terrain::Terrain()
	: GameObject()
{
}

//-----------------------------------------------------------------------------
// 		デストラクタ
//-----------------------------------------------------------------------------
Terrain::~Terrain()
{
	Term();
}

//-----------------------------------------------------------------------------
// 		初期化処理
//-----------------------------------------------------------------------------
bool Terrain::Init()
{
	return GameObject::Init();
}

//-----------------------------------------------------------------------------
// 		地形の初期化
//-----------------------------------------------------------------------------
bool Terrain::Init(
	const std::wstring& filePath,
	const std::wstring& fieldMapPath,
	float heightScale,
	float gridScale,
	DirectX::XMFLOAT3 centerPos
)
{
	if (!GameObject::Init())
	{
		return false;
	}

	m_HeightScale = heightScale;
	m_GridScale = gridScale;
	m_CenterPosition = centerPos;
	m_PipelineName = L"TerrainPipeline";

	// ハイトマップの読み込み
	if (!LoadHeightMap(filePath))
	{
		ELOG("Error : LoadHeightMap failed. path = %ls", filePath.c_str());
		return false;
	}

	// フィールドマップの読み込み
	if (!LoadFieldMap(fieldMapPath))
	{
		ELOG("Error : LoadFieldMap failed. path = %ls", fieldMapPath.c_str());
	}

	// DirectX12リソース・デバイスの取得
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	ComPtr<ID3D12Device> pDevice = resourceManager.GetDevice();
	DescriptorPool* pPool = resourceManager.GetPool(POOL_TYPE_RES);

	// 地形に使用するテクスチャの読み込み
	const std::wstring terrainTexturePath = L"Assets/Texture/Terrain/";
	SetTerrainTexture(TerrainTextureType::Grass, terrainTexturePath + L"grass.png");
	SetTerrainTexture(TerrainTextureType::Rock, terrainTexturePath + L"rock.jpg");
	LoadMacroTexture(L"Assets/Texture/Terrain/MacroMap.png");

	// リソースのチェック
	if (!pDevice || !pPool)
	{
		ELOG("Error : ResourceManager not ready.");
		return false;
	}

	// チャンクの頂点生成
	uint32_t chunkPixelsX = m_Width / kTerrainChunkDiv;
	uint32_t chunkPixelsZ = m_Height / kTerrainChunkDiv;

	// 地形全体の横幅と奥行きの計算
	float fullTerrainWidth = static_cast<float>(m_Width - 1) * m_GridScale;
	float fullTerrainHeight = static_cast<float>(m_Height - 1) * m_GridScale;

	// 地形の原点
	float terrainOriginX = m_CenterPosition.x - fullTerrainWidth * 0.5f;
	float terrainOriginZ = m_CenterPosition.z - fullTerrainHeight * 0.5f;

	// コマンドリストの初期化
	if (!m_InitCommandList.Init(pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, 1)) 
		return false;

	// フェンスの初期化
	if (!m_InitFence.Init(pDevice.Get())) 
		return false;

	// コマンドリストのリセット
	ID3D12GraphicsCommandList* pCmdList = m_InitCommandList.Reset();

	// ハイトマップテクスチャの作成
	if (!CreateHeightMapTexture(pDevice.Get(), pCmdList)) return false;

	// コマンドリストのクローズと実行
	pCmdList->Close();
	ID3D12CommandList* pLists[] = { pCmdList };
	resourceManager.GetQueue()->ExecuteCommandLists(1, pLists);
	m_InitFence.Sync(resourceManager.GetQueue().Get());

	// チャンクごとの頂点生成用定数バッファの初期化
	const uint32_t terrainGenCbSlots = kTerrainChunkDiv * kTerrainChunkDiv * kTerrainLodLevels;
	if (!m_TerrainGenCB.Init(pDevice.Get(), pPool, sizeof(CbTerrainGen), terrainGenCbSlots)) return false;

	m_Chunks.clear();
	const size_t numChunks = static_cast<size_t>(kTerrainChunkDiv * kTerrainChunkDiv);
	m_Chunks.reserve(numChunks);

	// チャンクごとに頂点バッファとインデックスバッファを作成
	for (uint32_t cz = 0; cz < kTerrainChunkDiv; ++cz)
	{
		for (uint32_t cx = 0; cx < kTerrainChunkDiv; ++cx)
		{
			auto chunk = std::make_unique<TerrainChunk>();
			uint32_t startX = cx * chunkPixelsX;
			uint32_t startZ = cz * chunkPixelsZ;

			float chunkWorldMinX = terrainOriginX + startX * m_GridScale;
			float chunkWorldMinZ = terrainOriginZ + startZ * m_GridScale;
			chunk->m_WorldCenter.x = chunkWorldMinX + (chunkPixelsX - 1) * m_GridScale * 0.5f;
			chunk->m_WorldCenter.z = chunkWorldMinZ + (chunkPixelsZ - 1) * m_GridScale * 0.5f;
			chunk->m_WorldCenter.y = m_CenterPosition.y;

			if (!CreateVertexBufferForChunk(pDevice.Get(), kLodSizes[0], kLodSizes[0], chunk.get())) return false;

			for (uint32_t lod = 0; lod < kTerrainLodLevels; ++lod)
			{
				auto indices = CreateIndices(kLodSizes[lod], kLodSizes[lod]);
				if (!chunk->m_IndexBuffers[lod].Init(pDevice.Get(), indices.size(), indices.data())) return false;
				chunk->m_IndexCounts[lod] = static_cast<uint32_t>(indices.size());
			}
			m_Chunks.push_back(std::move(chunk));
		}
	}

	// 定数バッファの作成
	if (!m_TransformCB.Init(pDevice.Get(), pPool, sizeof(CbTerrainTransform))) return false;
	if (!m_MaterialCB.Init(pDevice.Get(), pPool, sizeof(CbMaterial))) return false;
	if (!m_Material.Init(pDevice.Get(), pPool, sizeof(CbMaterial), 1)) return false;

	// マテリアル定数バッファの初期値設定
	auto pMaterialCB = m_MaterialCB.GetPtr<CbMaterial>();
	if (pMaterialCB)
	{
		pMaterialCB->BaseColor = { 0.8f, 0.8f, 0.8f };
		pMaterialCB->Alpha = 1.0f;
		pMaterialCB->HasBaseColorMap = 1;
	}

	// 地形の法線マップをマテリアルにセット
	DirectX::ResourceUploadBatch batch(pDevice.Get());
	batch.Begin();
	if (m_Material.SetTexture(0, TU_NORMAL, L"Assets/Texture/Terrain/HeightMap_normal.png", batch))
	{
		if (pMaterialCB) pMaterialCB->HasNormalMap = 1;
	}
	auto finish = batch.End(resourceManager.GetQueue().Get());
	finish.wait();

	return true;
}

//-----------------------------------------------------------------------------
//		終了処理
//-----------------------------------------------------------------------------
void Terrain::Term()
{
	m_InitCommandList.Term();
	m_InitFence.Term();
	m_TerrainGenCB.Term();

	auto pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (pPool)
	{
		if (m_HeightMapSRV) pPool->FreeHandle(m_HeightMapSRV);
		if (m_VertexBufferUAV) pPool->FreeHandle(m_VertexBufferUAV);
	}

	for (auto& chunk : m_Chunks)
	{
		if (!chunk) continue;
		for (uint32_t lod = 0; lod < kTerrainLodLevels; ++lod) chunk->m_IndexBuffers[lod].Term();
		chunk->m_VertexBuffer.Reset();
		if (chunk->m_VertexBufferUAV && pPool) pPool->FreeHandle(chunk->m_VertexBufferUAV);
	}
	m_Chunks.clear();

	m_VertexBufferResource.Reset();
	m_HeightMapTexture.Reset();
	m_HeightMapUploadBuffer.Reset();

	m_TransformCB.Term();
	m_MaterialCB.Term();
	m_Material.Term();

	if (m_FieldMapTexture) m_FieldMapTexture->Term();
	if (m_MacroTexture) m_MacroTexture->Term();

	for (auto& pair : m_TerrainTextures) if (pair.second) pair.second->Term();

	GameObject::Term();
}

//-----------------------------------------------------------------------------
// 		更新処理
//-----------------------------------------------------------------------------
void Terrain::Update(float deltaTime)
{
	GameObject::Update(deltaTime);

	auto camera = GameManager::GetScene()->GetCamera();

	// カメラの現在位置を取得
	DirectX::XMFLOAT3 cameraPos = camera->GetPosition();
	auto camP = DirectX::XMLoadFloat3(&cameraPos);

	// 全チャンクに対して、カメラとの距離から目標とするLODレベルを算出
	for (auto& chunk : m_Chunks)
	{
		// チャンクの中心位置をロード
		auto center = DirectX::XMLoadFloat3(&chunk->m_WorldCenter);
		// カメラとチャンク中心の距離を計算
		float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(camP, center)));

		// 定義された距離閾値に基づき LOD インデックスを決定
		if (dist >= kLodDistanceMid)       chunk->m_TargetLOD = 2; // 最も粗い
		else if (dist >= kLodDistanceNear) chunk->m_TargetLOD = 1; // 中間
		else                               chunk->m_TargetLOD = 0; // 最も精細
	}
}

//-----------------------------------------------------------------------------
// 		地形を描画
//-----------------------------------------------------------------------------
void Terrain::Draw(const RenderContext& context)
{
	if (!context.pCmdList) return;
	auto pCmdList = context.pCmdList;
	auto pDevice = ResourceManager::GetInstance().GetDevice();

	// --- Compute Phase: 必要に応じて頂点データを再生成 ---
	for (uint32_t z = 0; z < kTerrainChunkDiv; ++z)
	{
		for (uint32_t x = 0; x < kTerrainChunkDiv; ++x)
		{
			uint32_t i = z * kTerrainChunkDiv + x;
			auto& chunk = *m_Chunks[i];

			// 隣接チャンクの LOD 状態を確認（境界の隙間を埋める縫い合わせ処理に必要）
			uint32_t targetLeft   = (x > 0)						? m_Chunks[i - 1]->m_TargetLOD					: chunk.m_TargetLOD;
			uint32_t targetRight  = (x < kTerrainChunkDiv - 1)	? m_Chunks[i + 1]->m_TargetLOD					: chunk.m_TargetLOD;
			uint32_t targetBottom = (z > 0)						? m_Chunks[i- kTerrainChunkDiv]->m_TargetLOD	: chunk.m_TargetLOD;
			uint32_t targetTop    = (z < kTerrainChunkDiv - 1)	? m_Chunks[i + kTerrainChunkDiv]->m_TargetLOD	: chunk.m_TargetLOD;

			// 自己または隣接の LOD に変化があった場合、コンピュートシェーダーで頂点を更新
			if (chunk.m_TargetLOD	!= chunk.m_CurrentLOD	|| chunk.m_LODLeft	!= targetLeft	||
				chunk.m_LODRight	!= targetRight			|| chunk.m_LODTop	!= targetTop	||
				chunk.m_LODBottom	!= targetBottom			|| chunk.m_CurrentLOD == 999)
			{
				// 最新の状態に同期
				chunk.m_CurrentLOD	= chunk.m_TargetLOD;
				chunk.m_LODLeft		= targetLeft;
				chunk.m_LODRight	= targetRight;
				chunk.m_LODTop		= targetTop;
				chunk.m_LODBottom	= targetBottom;

				// CS実行用の定数バッファをセットアップ
				auto pGen = m_TerrainGenCB.GetPtr<CbTerrainGen>(i);
				if (pGen)
				{
					// LODに応じたグリッド解像度を取得
					uint32_t gridW			= kLodSizes[chunk.m_CurrentLOD];
					uint32_t gridH			= kLodSizes[chunk.m_CurrentLOD];
					uint32_t chunkPixelsX	= m_Width / kTerrainChunkDiv;
					uint32_t chunkPixelsZ	= m_Height / kTerrainChunkDiv;

					float chunkWorldW = static_cast<float>(chunkPixelsX) * m_GridScale;
					float chunkWorldH = static_cast<float>(chunkPixelsZ) * m_GridScale;

					pGen->CurrentLOD	= chunk.m_CurrentLOD;
					pGen->LODLeft		= chunk.m_LODLeft;
					pGen->LODRight		= chunk.m_LODRight;
					pGen->LODTop		= chunk.m_LODTop;
					pGen->LODBottom		= chunk.m_LODBottom;
					pGen->Width			= gridW;
					pGen->Height		= gridH;
					pGen->GridScale		= (chunkWorldW / (gridW - 1) + chunkWorldH / (gridH - 1)) * 0.5f;

					// ハイトマップ上のサンプリング開始座標
					pGen->HeightMapStartX		= x * chunkPixelsX;
					pGen->HeightMapStartZ		= z * chunkPixelsZ;
					pGen->HeightMapChunkPixelsX = chunkPixelsX;
					pGen->HeightMapChunkPixelsZ = chunkPixelsZ;
					pGen->HeightScale			= m_HeightScale;
					pGen->HeightOffset			= kHeightOffset;

					// チャンクの左上ワールド座標
					pGen->ChunkWorldPos = { 
						chunk.m_WorldCenter.x - chunkWorldW * 0.5f, 
						m_CenterPosition.y, 
						chunk.m_WorldCenter.z - chunkWorldH * 0.5f 
					};

					// コンピュートシェーダーをディスパッチして頂点バッファを書き換え
					ExecuteTerrainVertexGenComputeChunk(pDevice.Get(), pCmdList,
						pGen->HeightMapStartX, pGen->HeightMapStartZ, pGen->HeightMapChunkPixelsX,
						pGen->HeightMapChunkPixelsZ, pGen->Width, pGen->Height, pGen->ChunkWorldPos, &chunk, i);
				}
			}
		}
	}

	// レンダリングパスの設定
	auto pCB = m_TransformCB.GetPtr<CbTerrainTransform>();
	if (!pCB) return;

	// 変換行列を転置して定数バッファへ格納
	DirectX::XMStoreFloat4x4(&pCB->World, DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity()));
	DirectX::XMStoreFloat4x4(&pCB->View, DirectX::XMMatrixTranspose(context.viewMatrix));
	DirectX::XMStoreFloat4x4(&pCB->Proj, DirectX::XMMatrixTranspose(context.projMatrix));

	// 描画ステートのバインド
	auto& resourceManager = ResourceManager::GetInstance();
	if (!resourceManager.GetRenderSystem()->SetPipelineState(context, m_PipelineName)) return;

	// インプットアセンブラの設定
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 各種リソースのデスクリプタテーブルをセット
	pCmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParamIndex::TransformCB), m_TransformCB.GetHandleGPU());
	pCmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParamIndex::LightCB), context.lightCB);
	pCmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParamIndex::CameraCB), context.cameraCB);
	pCmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootParamIndex::MaterialCB), m_MaterialCB.GetHandleGPU());

	// テクスチャのバインド（Nullチェックを行いながら割り当て）
	auto bindTex = [&](RootParamIndex idx, D3D12_GPU_DESCRIPTOR_HANDLE h) {
		if (h.ptr) pCmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(idx), h);
		};

	bindTex(RootParamIndex::BaseColorMap, m_Material.GetTextureHandle(0, TU_BASE_COLOR));
	bindTex(RootParamIndex::NormalMap, m_Material.GetTextureHandle(0, TU_NORMAL));
	if (m_FieldMapTexture) bindTex(RootParamIndex::FieldMap, m_FieldMapTexture->GetHandleGPU());
	if (m_MacroTexture) bindTex(RootParamIndex::MacroTexture, m_MacroTexture->GetHandleGPU());

	// 地形テクスチャのマルチテクスチャバインド
	auto itGrass = m_TerrainTextures.find(TerrainTextureType::Grass);
	if (itGrass != m_TerrainTextures.end()) bindTex(RootParamIndex::GrassTexture, itGrass->second->GetHandleGPU());
	auto itRock = m_TerrainTextures.find(TerrainTextureType::Rock);
	if (itRock != m_TerrainTextures.end()) bindTex(RootParamIndex::RockTexture, itRock->second->GetHandleGPU());

	// 各チャンクのインデックスバッファを用いた描画コール
	for (const auto& chunk : m_Chunks)
	{
		// 999（未生成）の場合はスキップ
		if (chunk->m_CurrentLOD == 999) continue;

		// 現在の LOD に対応するインデックスバッファビューを取得
		auto ibv = chunk->m_IndexBuffers[chunk->m_CurrentLOD].GetView();
		pCmdList->IASetVertexBuffers(0, 1, &chunk->m_VertexBufferView);
		pCmdList->IASetIndexBuffer(&ibv);

		// インデックス描画の実行
		pCmdList->DrawIndexedInstanced(chunk->m_IndexCounts[chunk->m_CurrentLOD], 1, 0, 0, 0);
	}

	GameObject::Draw(context);
}

bool Terrain::LoadMacroTexture(const std::wstring& filePath)
{
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	ComPtr<ID3D12Device> pDevice = resourceManager.GetDevice();
	ComPtr<ID3D12CommandQueue> pQueue = resourceManager.GetQueue();
	DescriptorPool* pPool = resourceManager.GetPool(POOL_TYPE_RES);

	if (!pDevice || !pQueue || !pPool) return false;

	std::wstring findPath;
	if (!SearchFilePathW(filePath.c_str(), findPath))
	{
		ELOG("Error : MacroTexture not found. path = %ls", filePath.c_str());
		return false;
	}

	m_MacroTexture = std::make_unique<Texture>();
	DirectX::ResourceUploadBatch batch(pDevice.Get());
	batch.Begin();

	// カラーマップなのでsRGB(true)として読み込む
	if (!m_MacroTexture->Init(pDevice.Get(), pPool, findPath.c_str(), true, batch))
	{
		ELOG("Error : MacroTexture Init failed.");
		batch.End(pQueue.Get());
		return false;
	}

	auto finish = batch.End(pQueue.Get());
	finish.wait();
	return true;
}
//-----------------------------------------------------------------------------
// 		高さマップのロード
//-----------------------------------------------------------------------------
// 参考資料	https://pgming-ctrl.com/directx12/texture-wic/
bool Terrain::LoadHeightMap(const std::wstring& filePath)
{
	std::wstring fullPath;
	SearchFilePathW(filePath.c_str(), fullPath);

	UINT width = 0, height = 0;
	std::vector<uint16_t> pixelData;
	if (!LoadImageWithWIC16(fullPath.c_str(), width, height, pixelData))
		return false;

	m_Width = width;
	m_Height = height;
	m_HeightMap.resize(static_cast<size_t>(width) * height);
	const float invMax16 = 1.0f / 65535.0f; // 16ビットの最大値で正規化するための逆数 (0.0f ~ 1.0fに収めるため)
	for (size_t i = 0; i < pixelData.size(); ++i)
	{
		const float v01 = static_cast<float>(pixelData[i]) * invMax16;
		m_HeightMap[i] = 1.0f - v01; // 地形の高さは黒（0）を高く、白（1）を低くするため、反転させる
	}
	return true;
}

//-----------------------------------------------------------------------------
// 		フィールドマップのロード
//-----------------------------------------------------------------------------
bool Terrain::LoadFieldMap(const std::wstring& filePath)
{
	std::wstring fullpath;
	SearchFilePathW(filePath.c_str(), fullpath);

	UINT width = 0, height = 0;
	std::vector<uint8_t> pixelData;
	if (!LoadImageWithWIC(fullpath.c_str(), GUID_WICPixelFormat32bppRGBA, width, height, pixelData))
		return false;

	if (width != m_Width || height != m_Height)
	{
		ELOG("Error : FieldMap size (%u x %u) does not match HeightMap size (%u x %u)",
			width, height, m_Width, m_Height);
		return false;
	}

	const float colorPixelMax = 255.0f;
	const UINT colorBytes = 4;
	m_FieldMap.resize(width * height);
	for (UINT i = 0; i < width * height; ++i)
	{
		m_FieldMap[i] = XMFLOAT3(
			static_cast<float>(pixelData[i * colorBytes + 0]) / colorPixelMax,
			static_cast<float>(pixelData[i * colorBytes + 1]) / colorPixelMax,
			static_cast<float>(pixelData[i * colorBytes + 2]) / colorPixelMax
		);
	}

	// GPU テクスチャの作成
	ResourceManager& resourceManager = ResourceManager::GetInstance();
	ComPtr<ID3D12Device> pDevice = resourceManager.GetDevice();
	ComPtr<ID3D12CommandQueue> pQueue = resourceManager.GetQueue();
	DescriptorPool* pPool = resourceManager.GetPool(POOL_TYPE_RES);

	if (pDevice == nullptr || pQueue == nullptr || pPool == nullptr)
	{
		ELOG("Error : ResourceManager DirectX12 resources are invalid.");
		return false;
	}

	m_FieldMapTexture = std::make_unique<Texture>();

	// アップロードバッチの作成
	DirectX::ResourceUploadBatch batch(pDevice.Get());
	batch.Begin();

	// テクスチャの初期化（フィールドテクスチャは色マップなのでsRGB=false）
	if (!m_FieldMapTexture->Init(pDevice.Get(), pPool, fullpath.c_str(), false, batch))
	{
		ELOG("Error : FieldMap Texture Init failed. path = %ls", fullpath.c_str());
		batch.End(pQueue.Get());
		m_FieldMapTexture.reset();
		return false;
	}

	// アップロードの完了
	auto finish = batch.End(pQueue.Get());
	finish.wait();

	m_HasFieldMap = true;
	return true;
}

//-----------------------------------------------------------------------------
// 		指定座標の高さを取得
//-----------------------------------------------------------------------------
float Terrain::GetHeightAt(float worldX, float worldZ) const
{
	// 事前計算されたテレインの論理サイズを算出
	const float terrainWidth = static_cast<float>(m_Width - 1) * m_GridScale;
	const float terrainHeight = static_cast<float>(m_Height - 1) * m_GridScale;

	// ワールド空間におけるテレインの左下（ローカル原点）
	const float offsetX = m_CenterPosition.x - terrainWidth * 0.5f;
	const float offsetZ = m_CenterPosition.z - terrainHeight * 0.5f;

	// グリッド空間上の絶対座標へ変換
	const float gridX = (worldX - offsetX) / m_GridScale;
	const float gridZ = (worldZ - offsetZ) / m_GridScale;

	// --- C++20 std::clamp を使った配列外アクセスの防御 ---
	// 地形の範囲外（端）に居る場合は一番端のグリッドに丸めるか、デフォルト高さを返す
	if (gridX < 0.0f || gridZ < 0.0f || gridX >= (m_Width - 1.0f) || gridZ >= (m_Height - 1.0f))
	{
		// 範囲外であれば現在の基準高さをそのまま返す
		return m_CenterPosition.y;
	}

	// グリッドのインデックス (cellX, cellZ) と、セル内のオフセット (tx, tz) を計算
	const uint32_t cellX = static_cast<uint32_t>(gridX);
	const uint32_t cellZ = static_cast<uint32_t>(gridZ);
	const float tx = gridX - cellX;
	const float tz = gridZ - cellZ;

	// マス目の四隅のインデックス計算
	const uint32_t index00 = cellZ * m_Width + cellX;
	const uint32_t index10 = index00 + 1;
	const uint32_t index01 = index00 + m_Width;
	const uint32_t index11 = index01 + 1;

	// 四隅の高さを取得
	const float h00 = (m_HeightMap[index00] - kHeightOffset) * m_HeightScale;
	const float h10 = (m_HeightMap[index10] - kHeightOffset) * m_HeightScale;
	const float h01 = (m_HeightMap[index01] - kHeightOffset) * m_HeightScale;
	const float h11 = (m_HeightMap[index11] - kHeightOffset) * m_HeightScale;

	// 三角形ポリゴンに沿った補間 (Barycentric coordinates)
	float localHeight = 0.0f;
	if (tx + tz <= 1.0f)
	{
		// 左下三角形
		localHeight = h00 * (1.0f - tx - tz) + h10 * tx + h01 * tz;
	}
	else
	{
		// 右上三角形
		localHeight = h10 * (1.0f - tz) + h11 * (tx + tz - 1.0f) + h01 * (1.0f - tx);
	}

	return m_CenterPosition.y + localHeight;
}

//-----------------------------------------------------------------------------
// 		地形テクスチャの設定
//-----------------------------------------------------------------------------
bool Terrain::SetTerrainTexture(TerrainTextureType type, const std::wstring& texturePath)
{
	// リソースマネージャーの取得
	ResourceManager& resourceManager = ResourceManager::GetInstance();

	// DirectX12リソースの取得
	ComPtr<ID3D12Device>		pDevice = resourceManager.GetDevice();
	ComPtr<ID3D12CommandQueue>	pQueue	= resourceManager.GetQueue();
	DescriptorPool*				pPool	= resourceManager.GetPool(POOL_TYPE_RES);

	if(pDevice== nullptr || pQueue == nullptr || pPool == nullptr)
	{
		ELOG("Error : ResourceManager DirectX12 resources are invalid.");
		return false;
	}

	// ファイルパスを解決
	std::wstring findPath;
	if (!SearchFilePathW(texturePath.c_str(), findPath))
	{
		ELOG("Error : Texture file not found. path = %ls", texturePath.c_str());
		return false;
	}

	// テクスチャの作成
	std::unique_ptr<Texture> pTexture = std::make_unique<Texture>();

	// アップロードバッチの作成
	DirectX::ResourceUploadBatch batch(pDevice.Get());
	batch.Begin();

	// テクスチャの初期化
	if (!pTexture->Init(pDevice.Get(), pPool, findPath.c_str(), true, batch))
	{
		ELOG("Error : Texture Init failed. path = %ls", findPath.c_str());
		batch.End(pQueue.Get());
		return false;
	}

	// アップロードの完了
	auto finish = batch.End(pQueue.Get());
	finish.wait();

	// マップに登録
	m_TerrainTextures[type] = std::move(pTexture);

	return true;
}

//-----------------------------------------------------------------------------
// 		指定座標のフィールドマップカラーを取得
//-----------------------------------------------------------------------------
bool Terrain::GetFieldMapColor(float worldX, float worldZ, XMFLOAT3& outColor) const
{
	float terrainWidth = static_cast<float>(m_Width - 1) * m_GridScale;
	float terrainHeight = static_cast<float>(m_Height - 1) * m_GridScale;
	float offsetX = m_CenterPosition.x - terrainWidth * 0.5f;
	float offsetZ = m_CenterPosition.z - terrainHeight * 0.5f;

	float gridX = (worldX - offsetX) / m_GridScale;
	float gridZ = (worldZ - offsetZ) / m_GridScale;

	int px = static_cast<int>(gridX);
	int pz = static_cast<int>(gridZ);

	if (px < 0 || pz < 0 || px >= static_cast<int>(m_Width) || pz >= static_cast<int>(m_Height))
	{
		outColor = XMFLOAT3(0.0f, 0.0f, 0.0f);
		return false;
	}

	uint32_t idx = static_cast<uint32_t>(pz) * m_Width + static_cast<uint32_t>(px);
	outColor = m_FieldMap[idx];
	return true;
}

//-----------------------------------------------------------------------------
// 		地形のワールド座標境界を取得
//-----------------------------------------------------------------------------
void Terrain::GetTerrainWorldBounds(float& outMinX, float& outMaxX, float& outMinZ, float& outMaxZ) const
{
	// 事前計算されたテレインのサイズを算出
	float terrainWidth = static_cast<float>(m_Width - 1) * m_GridScale;
	float terrainHeight = static_cast<float>(m_Height - 1) * m_GridScale;
	float offsetX = m_CenterPosition.x - terrainWidth * 0.5f;
	float offsetZ = m_CenterPosition.z - terrainHeight * 0.5f;

	outMinX = offsetX;
	outMaxX = offsetX + terrainWidth;
	outMinZ = offsetZ;
	outMaxZ = offsetZ + terrainHeight;
}

D3D12_GPU_DESCRIPTOR_HANDLE Terrain::GetMacroTextureHandle() const
{
	if (m_MacroTexture) return m_MacroTexture->GetHandleGPU();
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

D3D12_GPU_DESCRIPTOR_HANDLE Terrain::GetHeightMapSRV() const
{
	if (m_HeightMapSRV) return m_HeightMapSRV->HandleGPU;
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

D3D12_GPU_DESCRIPTOR_HANDLE Terrain::GetFieldMapSRV() const
{
	if (m_FieldMapTexture) return m_FieldMapTexture->GetHandleGPU();
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

std::vector<uint32_t> Terrain::CreateIndices(uint32_t gridWidth, uint32_t gridHeight)
{
	if(gridWidth == 0) gridWidth = m_Width;
	if (gridHeight == 0) gridHeight = m_Height;

	std::vector<uint32_t> indices;
	uint32_t indexCount = (gridWidth - 1) * (gridHeight - 1) * 6;
	indices.reserve(indexCount);

	for (uint32_t z = 0; z < gridHeight - 1; ++z)
	{
		for (uint32_t x = 0; x < gridWidth - 1; ++x)
		{
			uint32_t topLeft = z * gridWidth + x;
			uint32_t topRight = topLeft + 1;
			uint32_t bottomLeft = (z + 1) * gridWidth + x;
			uint32_t bottomRight = bottomLeft + 1;
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}
	return indices;
}

bool Terrain::CreateHeightMapTexture(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList)
{
	if(m_HeightMap.empty() || m_Width == 0 || m_Height == 0)
		return false;

	const UINT64 uploadSize = static_cast<UINT64>(m_Width) * m_Height * sizeof(float);

	// 高さマップ用テクスチャ（Default、R32_FLOAT）
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_HeightMapTexture.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : CreateCommittedResource(HeightMap texture) failed. hr=0x%x", hr);
		return false;
	}

	// アップロード用バッファ（コマンド実行まで保持するためメンバで保持）
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Alignment = 0;
	bufDesc.Width = uploadSize;
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&bufDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_HeightMapUploadBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : CreateCommittedResource(HeightMap upload) failed. hr=0x%x", hr);
		return false;
	}

	void* pMapped = nullptr;
	hr = m_HeightMapUploadBuffer->Map(0, nullptr, &pMapped);
	if (FAILED(hr) || !pMapped)
	{
		ELOG("Error : HeightMap upload Map failed. hr=0x%x", hr);
		return false;
	}
	memcpy(pMapped, m_HeightMap.data(), static_cast<size_t>(uploadSize));
	m_HeightMapUploadBuffer->Unmap(0, nullptr);

	// コピー用レイアウト取得
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	UINT numRows = 0;
	UINT64 rowSize = 0;
	UINT64 totalBytes = 0;
	pDevice->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &totalBytes);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_HeightMapTexture.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
	dstLoc.pResource = m_HeightMapTexture.Get();
	dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
	srcLoc.pResource = m_HeightMapUploadBuffer.Get();
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLoc.PlacedFootprint = footprint;

	pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
	pCmdList->ResourceBarrier(1, &barrier);

	// SRV 作成（プールから1つ取得）
	DescriptorPool* pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (!pPool)
	{
		ELOG("Error : GetPool(POOL_TYPE_RES) failed.");
		return false;
	}
	m_HeightMapSRV = pPool->AllocHandle();
	if (!m_HeightMapSRV)
	{
		ELOG("Error : AllocHandle for HeightMap SRV failed.");
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	pDevice->CreateShaderResourceView(m_HeightMapTexture.Get(), &srvDesc, m_HeightMapSRV->HandleCPU);

	return true;
}

//-----------------------------------------------------------------------------
// 		頂点バッファを Compute Shader で書き込むためのリソースを作成
//-----------------------------------------------------------------------------
bool Terrain::CreateVertexBufferForCompute(ID3D12Device* pDevice)
{
	if (m_Width == 0 || m_Height == 0)
		return false;

	// 頂点バッファのサイズを計算
	const UINT64 vertexBufferSize = static_cast<UINT64>(m_Width) * m_Height * sizeof(TerrainVertex);

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Alignment = 0;
	bufDesc.Width = vertexBufferSize;
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// UAVとして書き込むためのバッファを作成
	HRESULT hr = pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&bufDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_VertexBufferResource.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : CreateCommittedResource(VertexBuffer UAV) failed. hr=0x%x", hr);
		return false;
	}

	m_VertexBufferView.BufferLocation = m_VertexBufferResource->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(TerrainVertex);
	m_VertexBufferView.SizeInBytes = static_cast<UINT>(vertexBufferSize);

	DescriptorPool* pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (!pPool)
	{
		ELOG("Error : GetPool(POOL_TYPE_RES) failed.");
		return false;
	}
	m_VertexBufferUAV = pPool->AllocHandle();
	if (!m_VertexBufferUAV)
	{
		ELOG("Error : AllocHandle for VertexBuffer UAV failed.");
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_Width * m_Height;
	uavDesc.Buffer.StructureByteStride = sizeof(TerrainVertex);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	pDevice->CreateUnorderedAccessView(m_VertexBufferResource.Get(), nullptr, &uavDesc, m_VertexBufferUAV->HandleCPU);

	return true;
}

bool Terrain::ExecuteTerrainVertexGenCompute(ID3D12Device* pDevice, ID3D12CommandQueue* pQueue, ID3D12GraphicsCommandList* pCmdList)
{
	DescriptorPool* pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (!BindComputePipeline(pCmdList, L"TerrainVertexGenPipeline", pPool))
	{
		ELOG("Error : BindComputePipeline TerrainVertexGenPipeline failed.");
		return false;
	}

	// b0: 定数バッファ
	D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = m_TerrainGenCB.GetHandleGPU();
	if (cbHandle.ptr == 0)
	{
		ELOG("Error : TerrainGenCB GetHandleGPU is 0.");
		return false;
	}
	pCmdList->SetComputeRootDescriptorTable(0, cbHandle);

	// t0: 高さマップ SRV
	if (!m_HeightMapSRV || m_HeightMapSRV->HandleGPU.ptr == 0)
	{
		ELOG("Error : HeightMap SRV invalid.");
		return false;
	}
	pCmdList->SetComputeRootDescriptorTable(1, m_HeightMapSRV->HandleGPU);

	// u0: 頂点バッファ UAV
	if (!m_VertexBufferUAV || m_VertexBufferUAV->HandleGPU.ptr == 0)
	{
		ELOG("Error : VertexBuffer UAV invalid.");
		return false;
	}
	pCmdList->SetComputeRootDescriptorTable(2, m_VertexBufferUAV->HandleGPU);

	const UINT groupX = (m_Width + 7) / 8;
	const UINT groupY = (m_Height + 7) / 8;
	pCmdList->Dispatch(groupX, groupY, 1);

	// 頂点バッファを VB 用に Transition（描画で使うため）
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_VertexBufferResource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCmdList->ResourceBarrier(1, &barrier);

	pCmdList->Close();

	ID3D12CommandList* pList = pCmdList;
	pQueue->ExecuteCommandLists(1, &pList);
	m_InitFence.Sync(pQueue);

	return true;
}

bool Terrain::CreateVertexBufferForChunk(
	ID3D12Device* pDevice, 
	uint32_t gridWidth, 
	uint32_t gridHeight, 
	TerrainChunk* pOut)
{
	if (!pOut || gridWidth == 0 || gridHeight == 0) return false;

	const UINT64 vertexBufferSize = static_cast<UINT64>(gridWidth) * gridHeight * sizeof(TerrainVertex);

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Alignment = 0;
	bufDesc.Width = vertexBufferSize;
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = pDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, // ← COMMON から変更
		nullptr,
		IID_PPV_ARGS(pOut->m_VertexBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { ELOG("CreateCommittedResource(Chunk VB) failed. hr=0x%x", hr); return false; }

	pOut->m_VertexBufferView.BufferLocation = pOut->m_VertexBuffer->GetGPUVirtualAddress();
	pOut->m_VertexBufferView.StrideInBytes = sizeof(TerrainVertex);
	pOut->m_VertexBufferView.SizeInBytes = static_cast<UINT>(vertexBufferSize);

	DescriptorPool* pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	pOut->m_VertexBufferUAV = pPool->AllocHandle();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = gridWidth * gridHeight;
	uavDesc.Buffer.StructureByteStride = sizeof(TerrainVertex);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	pDevice->CreateUnorderedAccessView(pOut->m_VertexBuffer.Get(), nullptr, &uavDesc, pOut->m_VertexBufferUAV->HandleCPU);

	return true;
}

bool Terrain::ExecuteTerrainVertexGenComputeChunk(
	ID3D12Device* pDevice, 
	ID3D12GraphicsCommandList* pCmdList, 
	uint32_t heightMapStartX, 
	uint32_t heightMapStartZ, 
	uint32_t chunkPixelsX, 
	uint32_t chunkPixelsZ, 
	uint32_t gridWidth, 
	uint32_t gridHeight, 
	const XMFLOAT3& chunkWorldOrigin, 
	TerrainChunk* pChunk,
	uint32_t cbSlotIndex)
{
	DescriptorPool* pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (!BindComputePipeline(pCmdList, L"TerrainVertexGenPipeline", pPool)) return false;

	// CbTerrainGen は呼び出し元（Draw内）で更新済みと仮定するため、ここでは CBV/SRV/UAV のバインドのみ行う
	pCmdList->SetComputeRootDescriptorTable(0, m_TerrainGenCB.GetHandleGPU(cbSlotIndex));
	pCmdList->SetComputeRootDescriptorTable(1, m_HeightMapSRV->HandleGPU);
	pCmdList->SetComputeRootDescriptorTable(2, pChunk->m_VertexBufferUAV->HandleGPU);

	// バリア: VBとして使っていたものを UAV (書き込み用) に遷移
	D3D12_RESOURCE_BARRIER barrierBefore = {};
	barrierBefore.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierBefore.Transition.pResource = pChunk->m_VertexBuffer.Get();
	barrierBefore.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrierBefore.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	pCmdList->ResourceBarrier(1, &barrierBefore);

	UINT gx = (gridWidth + 7) / 8, gy = (gridHeight + 7) / 8;
	pCmdList->Dispatch(gx, gy, 1);

	// バリア: 書き込み終わったので VB に戻す
	D3D12_RESOURCE_BARRIER barrierAfter = {};
	barrierAfter.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierAfter.Transition.pResource = pChunk->m_VertexBuffer.Get();
	barrierAfter.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrierAfter.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	pCmdList->ResourceBarrier(1, &barrierAfter);

	return true;
}

