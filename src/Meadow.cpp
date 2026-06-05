#include "Meadow.h"
#include "Scene.h"
#include "Terrain.h"
#include "Camera.h"
#include "ModelManager.h"
#include "ResourceManager.h"
#include "RenderSystem.h"
#include "ComputeUtil.h"
#include "Logger.h"
#include "Wind.h"

namespace
{
	static constexpr float kCullingMaxDistance = 150.0f; // 草のカリング距離
	static constexpr float kGrassChunkSize = 30.0f; // 草チャンクのサイズ
	constexpr uint32_t kMaxGrassPerChunk = 200;
	constexpr uint32_t kMaxVisibleChunks = 100;
	constexpr float kTerrainHeightScale = 2000.0f;

}

Meadow::Meadow() 
	: GameObject() 
{
}

Meadow::~Meadow() 
{ 
	Term();
}

bool Meadow::Init() 
{ 
	return true; 
}

bool Meadow::Init(Scene* scene, Terrain* terrain)
{
	m_Scene = scene;
	m_Terrain = terrain;

	if (!ModelManager::GetInstance().LoadModel(m_ModelPath)) return false;

	auto pDevice = ResourceManager::GetInstance().GetDevice();
	auto pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);

	if (!m_CbGrassNear.Init(pDevice.Get(), pPool, sizeof(CbWind))) return false;

	// チャンクごとに定数バッファを切り替えてCSを呼ぶため、多めにスロット(100個)を確保
	if (!m_CbGeneration.Init(pDevice.Get(), pPool, sizeof(CbGenerationParams), 100)) return false;

	// 代わりに、最大表示数分の空っぽのGPUバッファだけを用意する
	if (!CreateGPUResources()) return false;

	return true;
}

void Meadow::Term()
{
	auto pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	if (pPool) {
		if (m_VisibleGrassUAV) pPool->FreeHandle(m_VisibleGrassUAV);
		if (m_DrawArgsUAV) pPool->FreeHandle(m_DrawArgsUAV);
	}
	m_CbGrassNear.Term();
	m_CbGeneration.Term();
}

bool Meadow::CreateGPUResources()
{
	auto pDevice = ResourceManager::GetInstance().GetDevice();
	auto pPool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	UINT64 bufferSize = sizeof(GrassNearData) * m_MaxGrassCapacity;

	D3D12_HEAP_PROPERTIES hpUpload = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_HEAP_PROPERTIES hpDefault = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC descBuffer = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, bufferSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };

	// 1. VisibleGrass (空のUAVバッファ)
	pDevice->CreateCommittedResource(&hpDefault, D3D12_HEAP_FLAG_NONE, &descBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&m_VisibleGrassBuffer));

	// 2. DrawArgs (UAV兼間接描画引数)
	descBuffer.Width = sizeof(D3D12_DRAW_ARGUMENTS);
	pDevice->CreateCommittedResource(&hpDefault, D3D12_HEAP_FLAG_NONE, &descBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, nullptr, IID_PPV_ARGS(&m_DrawArgsBuffer));

	// 3. ArgsReset (リセット用)
	descBuffer.Flags = D3D12_RESOURCE_FLAG_NONE;
	pDevice->CreateCommittedResource(&hpUpload, D3D12_HEAP_FLAG_NONE, &descBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ArgsResetBuffer));

	D3D12_DRAW_ARGUMENTS initArgs = { 0, 1, 0, 0 };
	void* pData;
	m_ArgsResetBuffer->Map(0, nullptr, &pData);
	memcpy(pData, &initArgs, sizeof(D3D12_DRAW_ARGUMENTS));

	// UAV作成
	m_VisibleGrassUAV = pPool->AllocHandle();
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = m_MaxGrassCapacity;
	uavDesc.Buffer.StructureByteStride = sizeof(GrassNearData);
	pDevice->CreateUnorderedAccessView(m_VisibleGrassBuffer.Get(), nullptr, &uavDesc, m_VisibleGrassUAV->HandleCPU);

	m_DrawArgsUAV = pPool->AllocHandle();
	uavDesc.Buffer.NumElements = 4;
	uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);
	pDevice->CreateUnorderedAccessView(m_DrawArgsBuffer.Get(), nullptr, &uavDesc, m_DrawArgsUAV->HandleCPU);

	// Command Signature作成
	D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
	argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
	D3D12_COMMAND_SIGNATURE_DESC csDesc = {};
	csDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
	csDesc.NumArgumentDescs = 1;
	csDesc.pArgumentDescs = &argDesc;
	pDevice->CreateCommandSignature(&csDesc, nullptr, IID_PPV_ARGS(&m_CommandSignature));

	return true;
}

void Meadow::Update(float deltaTime)
{
	if (!m_Scene || !m_Terrain) return;
	Camera* pCam = m_Scene->GetCamera();
	if (!pCam) return;

	// ==========================================
	// チャンクマクロカリング（CPU側）
	// ==========================================
	const int chunkRadius = static_cast<int>(std::ceil(kCullingMaxDistance / kGrassChunkSize));

	XMFLOAT3 camPos = pCam->GetPosition();
	int centerChunkX = static_cast<int>(std::floor(camPos.x / kGrassChunkSize));
	int centerChunkZ = static_cast<int>(std::floor(camPos.z / kGrassChunkSize));

	m_VisibleChunks.clear();

	// カメラ周辺のチャンクを走査し、視界内・距離内のチャンク座標だけをリスト化する
	for (int z = -chunkRadius; z <= chunkRadius; ++z)
	{
		for (int x = -chunkRadius; x <= chunkRadius; ++x)
		{
			float chunkStartX = (centerChunkX + x) * kGrassChunkSize;
			float chunkStartZ = (centerChunkZ + z) * kGrassChunkSize;

			float cX = chunkStartX + kGrassChunkSize * 0.5f;
			float cZ = chunkStartZ + kGrassChunkSize * 0.5f;
			float distSq = (cX - camPos.x) * (cX - camPos.x) + (cZ - camPos.z) * (cZ - camPos.z);

			float cullDist = kCullingMaxDistance + kGrassChunkSize * 0.707f;
			if (distSq > cullDist * cullDist) continue;

			m_VisibleChunks.push_back({ XMFLOAT2(chunkStartX, chunkStartZ) });
		}
	}

	// 風の更新
	CbWind* pCbWind = m_CbGrassNear.GetPtr<CbWind>();
	Wind* pWind = m_Scene->GetGameObjectByName<Wind>("Wind");
	if (pCbWind) *pCbWind = (pWind) ? pWind->GetCbWind() : CbWind{};
}

void Meadow::Draw(const RenderContext& context)
{
	if (m_VisibleChunks.empty() || !context.pCmdList) return;
	auto pCmd = context.pCmdList;

	// ==========================================
	// 1. 引数バッファをリセット
	// ==========================================
	D3D12_RESOURCE_BARRIER bCopy[1] = {};
	bCopy[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bCopy[0].Transition.pResource = m_DrawArgsBuffer.Get();
	bCopy[0].Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	bCopy[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	pCmd->ResourceBarrier(1, bCopy);

	pCmd->CopyBufferRegion(m_DrawArgsBuffer.Get(), 0, m_ArgsResetBuffer.Get(), 0, sizeof(D3D12_DRAW_ARGUMENTS));

	// ==========================================
	// 2. CSでチャンクごとに草をGPU生成
	// ==========================================
	D3D12_RESOURCE_BARRIER bCS[2] = {};
	bCS[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bCS[0].Transition.pResource = m_DrawArgsBuffer.Get();
	bCS[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	bCS[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	bCS[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bCS[1].Transition.pResource = m_VisibleGrassBuffer.Get();
	bCS[1].Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	bCS[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	pCmd->ResourceBarrier(2, bCS);

	BindComputePipeline(pCmd, L"GrassGeneratorPipeline", ResourceManager::GetInstance().GetPool(POOL_TYPE_RES));
	XMFLOAT4X4 viewProj;
	// View * Proj の順番で乗算し、HLSL用に転置する
	XMStoreFloat4x4(&viewProj, XMMatrixTranspose(context.viewMatrix * context.projMatrix));

	float tMinX, tMaxX, tMinZ, tMaxZ;
	m_Terrain->GetTerrainWorldBounds(tMinX, tMaxX, tMinZ, tMaxZ);

	const uint32_t grassPerChunk = kMaxGrassPerChunk; // 1チャンク(30x30m)あたりに生成を試みる最大の草の数。密度を上げたい場合はこの数値を増やします。
	Camera* pCam = m_Scene->GetCamera();
	for (size_t i = 0; i < m_VisibleChunks.size(); ++i)
	{
		if (i >= kMaxVisibleChunks) break; // スロット上限安全装置
		CbGenerationParams* pParams = m_CbGeneration.GetPtr<CbGenerationParams>(static_cast<uint32_t>(i));
		if (pParams)
		{
			pParams->ViewProj = viewProj;
			pParams->CameraPos = pCam->GetPosition();
			pParams->MaxDistance = kCullingMaxDistance;

			pParams->ChunkBasePos = m_VisibleChunks[i].BasePos;
			pParams->ChunkSize = kGrassChunkSize;

			pParams->TerrainBasePos = XMFLOAT2(tMinX, tMinZ);
			pParams->TerrainWidth = tMaxX - tMinX;
			pParams->TerrainHeight = tMaxZ - tMinZ;
			pParams->TerrainBaseY = 0.0f;
			pParams->HeightScale = 2000.0f; // Terrainに合わせて調整
			pParams->HeightOffset = 1.0f;
		}

		pCmd->SetComputeRootDescriptorTable(0, m_CbGeneration.GetHandleGPU(static_cast<uint32_t>(i)));
		pCmd->SetComputeRootDescriptorTable(1, m_Terrain->GetHeightMapSRV());
		pCmd->SetComputeRootDescriptorTable(2, m_Terrain->GetFieldMapSRV());
		pCmd->SetComputeRootDescriptorTable(3, m_VisibleGrassUAV->HandleGPU);
		pCmd->SetComputeRootDescriptorTable(4, m_DrawArgsUAV->HandleGPU);

		// 指定した数の草を生成するようGPUに命令
		pCmd->Dispatch((grassPerChunk + 63) / 64, 1, 1);
	}

	// ==========================================
	// 3. ExecuteIndirectによる爆速描画
	// ==========================================
	D3D12_RESOURCE_BARRIER bDraw[2] = {};
	bDraw[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bDraw[0].Transition.pResource = m_DrawArgsBuffer.Get();
	bDraw[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	bDraw[0].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	bDraw[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bDraw[1].Transition.pResource = m_VisibleGrassBuffer.Get();
	bDraw[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	bDraw[1].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	pCmd->ResourceBarrier(2, bDraw);

	if (!ResourceManager::GetInstance().GetRenderSystem()->SetPipelineState(context, L"GrassNear"))
	{
		ELOG("Error: Failed to set pipeline state for GrassNear");
	}

	pCmd->SetGraphicsRootDescriptorTable(0, m_CbGeneration.GetHandleGPU(0));

	auto mat = &ModelManager::GetInstance().GetModel(m_ModelPath)->material;
	if (mat->GetTextureHandle(0, TU_BASE_COLOR).ptr)
		pCmd->SetGraphicsRootDescriptorTable(1, mat->GetTextureHandle(0, TU_BASE_COLOR)); // インデックス1

	pCmd->SetGraphicsRootDescriptorTable(2, m_CbGrassNear.GetHandleGPU()); // インデックス2

	if (m_Terrain->GetMacroTextureHandle().ptr)
		pCmd->SetGraphicsRootDescriptorTable(3, m_Terrain->GetMacroTextureHandle()); // インデックス3

	// VBVの設定とExecuteIndirect
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = m_VisibleGrassBuffer->GetGPUVirtualAddress();
	vbv.SizeInBytes = sizeof(GrassNearData) * m_MaxGrassCapacity;
	vbv.StrideInBytes = sizeof(GrassNearData);

	pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pCmd->IASetVertexBuffers(0, 1, &vbv);

	pCmd->ExecuteIndirect(m_CommandSignature.Get(), 1, m_DrawArgsBuffer.Get(), 0, nullptr, 0);
}