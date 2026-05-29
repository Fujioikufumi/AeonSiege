#include "MaskedSprite.h"
#include "ResourceManager.h"
#include "FileUtil.h"
#include "Logger.h"
#include "ResourceUploadBatch.h"
#include "PipelineStateManager.h"
#include "NameSpace.h"
#include <DirectXHelpers.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

MaskedSprite::MaskedSprite(GameObject* obj)
	: Component(obj)
{
	m_ComponentName = "MaskedSprite";
}

MaskedSprite::~MaskedSprite()
{
	Term();
}

bool MaskedSprite::Init()
{
	return true;
}

bool MaskedSprite::Init(const std::wstring& texturePath)
{
	m_TexturePath = texturePath;

	std::wstring fullPath;
	if (!SearchFilePath(texturePath.c_str(), fullPath))
	{
		ELOG("MaskedSprite: Texture file not found: %ls", texturePath.c_str());
		return false;
	}

	auto device = GetDevice();
	auto pool = GetPool(POOL_TYPE_RES);
	auto queue = GetQueue();

	if (!device || !pool || !queue)
	{
		ELOG("MaskedSprite: ResourceManager not initialized");
		return false;
	}

	// テクスチャの読み込み
	m_Texture = std::make_unique<Texture>();
	ResourceUploadBatch batch(device.Get());
	batch.Begin();
	if (!m_Texture->Init(device.Get(), pool, fullPath.c_str(), true, batch))
	{
		ELOG("MaskedSprite: Texture::Init failed: %ls", fullPath.c_str());
		return false;
	}
	auto finish = batch.End(queue.Get());
	finish.wait();

	// 頂点バッファの作成
	CreateVertexBuffer();

	// 定数バッファの作成
	if (!m_SpriteCB.Init(device.Get(), pool, sizeof(SpriteBuffer)))
	{
		ELOG("MaskedSprite: SpriteCB init failed");
		return false;
	}
	if (!m_MaskCB.Init(device.Get(), pool, sizeof(MaskBuffer)))
	{
		ELOG("MaskedSprite: MaskCB init failed");
		return false;
	}

	UpdateConstantBuffers();
	return true;
}

void MaskedSprite::Term()
{
	m_MaskCB.Term();
	m_SpriteCB.Term();
	m_VertexBuffer.Term();

	if (m_Texture)
	{
		m_Texture->Term();
		m_Texture.reset();
	}
}

void MaskedSprite::Update(float deltaTime)
{
	UpdateConstantBuffers();
}

void MaskedSprite::Draw(const RenderContext& context)
{
	if (!m_Texture || !context.pCmdList)
		return;

	// MaskedUIPipelineの描画パイプラインを取得
	auto* pipelineInfo = PipelineStateManager::GetInstance().GetPipelineState(L"MaskedUIPipeline");
	if (!pipelineInfo || !pipelineInfo->isValid)
	{
		ELOG("MaskedSprite: MaskedUIPipeline not found or invalid");
		return;
	}

	// GPUテクスチャハンドルの取得
	D3D12_GPU_DESCRIPTOR_HANDLE texHandle = m_Texture->GetHandleGPU();
	if (texHandle.ptr == 0)
	{
		ELOG("MaskedSprite: Texture handle invalid");
		return;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE spriteCb = m_SpriteCB.GetHandleGPU();
	D3D12_GPU_DESCRIPTOR_HANDLE maskCb = m_MaskCB.GetHandleGPU();
	if (spriteCb.ptr == 0 || maskCb.ptr == 0)
	{
		ELOG("MaskedSprite: ConstantBuffer handle invalid");
		return;
	}

	context.pCmdList->SetPipelineState(pipelineInfo->pPSO.Get());
	context.pCmdList->SetGraphicsRootSignature(pipelineInfo->rootSig.GetPtr());
	context.pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	auto vbv = m_VertexBuffer.GetView();
	context.pCmdList->IASetVertexBuffers(0, 1, &vbv);

	// 0: b0 (VS) SpriteCB
	// 1: b1 (PS) MaskCB
	// 2: t0 (PS) Texture
	context.pCmdList->SetGraphicsRootDescriptorTable(0, spriteCb);
	context.pCmdList->SetGraphicsRootDescriptorTable(1, maskCb);
	context.pCmdList->SetGraphicsRootDescriptorTable(2, texHandle);

	context.pCmdList->DrawInstanced(4, 1, 0, 0);
}

void MaskedSprite::SetUV(const XMFLOAT2& uvMin, const XMFLOAT2& uvMax)
{
	m_UVMin = uvMin;
	m_UVMax = uvMax;
	CreateVertexBuffer();
}

void MaskedSprite::SetMaskRectUV(float u0, float v0, float u1, float v1)
{
	m_MaskRectUV = XMFLOAT4(u0, v0, u1, v1);
}

void MaskedSprite::CreateVertexBuffer()
{
	struct Vertex
	{
		XMFLOAT2 Position;
		XMFLOAT2 TexCoord;
	};

	Vertex vertices[4] =
	{
		{ { -1.0f,  1.0f }, { m_UVMin.x, m_UVMax.y } }, // 左上
		{ {  1.0f,  1.0f }, { m_UVMax.x, m_UVMax.y } }, // 右上
		{ { -1.0f, -1.0f }, { m_UVMin.x, m_UVMin.y } }, // 左下
		{ {  1.0f, -1.0f }, { m_UVMax.x, m_UVMin.y } }, // 右下
	};

	auto device = GetDevice();
	if (!device)
		return;

	if (!m_VertexBuffer.Init<Vertex>(device.Get(), 4, vertices))
	{
		ELOG("MaskedSprite: VertexBuffer init failed");
	}
}

void MaskedSprite::UpdateConstantBuffers()
{
	const XMFLOAT2 screenSize = { static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) };

	// SpriteCB (b0)
	SpriteBuffer* sb = m_SpriteCB.GetPtr<SpriteBuffer>();
	if (!sb)
	{
		ELOG("MaskedSprite: SpriteCB GetPtr failed");
		return;
	}

	// 定数バッファに値をセット
	sb->Position_Padding = XMFLOAT4(m_Position.x, m_Position.y, 0.0f, 0.0f);
	sb->Size_Padding = XMFLOAT4(m_Size.x, m_Size.y, 0.0f, 0.0f);
	sb->Scale_Padding = XMFLOAT4(m_Scale.x, m_Scale.y, 0.0f, 0.0f);
	sb->Rotation_Anchor_Padding = XMFLOAT4(m_Rotation, 0.0f, 0.0f, 0.0f);
	sb->ScreenSize_Padding = XMFLOAT4(screenSize.x, screenSize.y, 0.0f, 0.0f);
	sb->Color = m_Color;

	// MaskCB (b1)
	MaskBuffer* maskBuffer = m_MaskCB.GetPtr<MaskBuffer>();
	if (!maskBuffer)
	{
		ELOG("MaskedSprite: MaskCB GetPtr failed");
		return;
	}

	// 進捗率は0.0～1.0の範囲にクランプ
	const float progress = std::clamp(m_Progress, 0.0f, 1.0f);

	// フェザーは0.0以上の値にクランプ
	const float feather = max(0.0f, m_Feather);

	// マスクモード
	const float mode = static_cast<float>(static_cast<int>(m_MaskMode));

	maskBuffer->MaskRectUV = m_MaskRectUV;
	maskBuffer->Params = XMFLOAT4(progress, feather, mode, 0.0f);
}