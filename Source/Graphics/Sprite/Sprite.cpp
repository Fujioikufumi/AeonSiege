#include "Graphics/Sprite/Sprite.h"
#include "Graphics/D3D12/ResourceManager.h"
#include "Utility/FileUtil.h"
#include "Utility/Logger.h"
#include "ResourceUploadBatch.h"
#include "Graphics/Renderer/PipelineStateManager.h"
#include <DirectXHelpers.h>
#include "Core/NameSpace.h"
//----------------------------------------------------------------------
//		2Dスプライトコンポーネント
//----------------------------------------------------------------------
Sprite::Sprite(GameObject* obj)
    : Component(obj), m_Position({0.0f, 0.0f}), m_Size({100.0f, 100.0f}), m_Scale({1.0f, 1.0f}), m_Rotation(0.0f), m_UVMin({0.0f, 0.0f}), m_UVMax({1.0f, 1.0f}), m_Color({1.0f, 1.0f, 1.0f, 1.0f})
{
	m_ComponentName = "Sprite";
}

Sprite::~Sprite()
{
	Term();
}

bool Sprite::Init()
{
	return true;
}

//----------------------------------------------------------------------
//		初期化
//----------------------------------------------------------------------
bool Sprite::Init(const std::wstring& texturePath)
{
	if (m_Texture != nullptr)
	{
		if (m_TexturePath == texturePath)
		{
			return true;
		}
		ELOG("Error : Sprite is already initialized. current = %ls, requested = %ls",
		     m_TexturePath.c_str(),
		     texturePath.c_str());
		return false;
	}
	m_TexturePath = texturePath;

	// ファイルパスの確認
	std::wstring fullPath;
	if (!SearchFilePath(texturePath.c_str(), fullPath))
	{
		ELOG("Error : Texture file not found: %ls", texturePath.c_str());
		return false;
	}

	// リソースの取得
	auto device = GetDevice();
	auto pool   = GetPool(POOL_TYPE_RES);
	auto queue  = GetQueue();

	if (!device || !pool || !queue)
	{
		ELOG("Error : ResourceManager not initialized");
		return false;
	}

	// テクスチャの作成
	m_Texture = ResourceManager::GetInstance().LoadTexture(texturePath, true);
	if (m_Texture == nullptr)
	{
		ELOG("Error : ResourceManager::LoadTexture() Failed. path = %ls", texturePath.c_str());
		return false;
	}

	// 頂点バッファの作成
	CreateVertexBuffer();

	// 定数バッファの作成（多重化）
	if (!m_ConstantBuffer.Init(device.Get(), pool, sizeof(SpriteBuffer), kFrameCount))
	{
		ELOG("Error : ConstantBuffer::Init() Failed.");
		return false;
	}

	return true;
}

//----------------------------------------------------------------------
//		終了処理
//----------------------------------------------------------------------
void Sprite::Term()
{
	m_ConstantBuffer.Term();

	for (auto i = 0u; i < kFrameCount; ++i)
	{
		m_VertexBuffer[i].Term();
	}

	m_Texture.reset();
}

//----------------------------------------------------------------------
//		更新処理
//----------------------------------------------------------------------
void Sprite::Update(float deltaTime)
{
	// UpdateConstantBuffer();
}

//----------------------------------------------------------------------
//		描画処理
//----------------------------------------------------------------------
void Sprite::Draw(const RenderContext& context)
{
	if (!m_Texture || !context.pCmdList)
		return;

	const uint32_t frame = context.frameIndex;

	// 現在のフレームスロットのCBを更新
	UpdateConstantBuffers(frame);

	// 現在のフレームスロットの頂点バッファにUVを反映
	{
		struct Vertex
		{
			XMFLOAT2 Position;
			XMFLOAT2 TexCoord;
		};
		Vertex* v = m_VertexBuffer[frame].Map<Vertex>();
		if (v)
		{
			v[0].TexCoord = {m_UVMin.x, m_UVMax.y}; // 左上
			v[1].TexCoord = {m_UVMax.x, m_UVMax.y}; // 右上
			v[2].TexCoord = {m_UVMin.x, m_UVMin.y}; // 左下
			v[3].TexCoord = {m_UVMax.x, m_UVMin.y}; // 右下
			m_VertexBuffer[frame].Unmap();
		}
	}

	auto* pipelineInfo = PipelineStateManager::GetInstance().GetPipelineState(L"UIPipeline");
	if (!pipelineInfo || !pipelineInfo->isValid)
	{
		ELOG("Error : UiPipeline not found or invalid");
		return;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = m_Texture->GetHandleGPU();
	if (textureHandle.ptr == 0)
	{
		ELOG("Error : Texture handle is invalid");
		return;
	}

	context.pCmdList->SetPipelineState(pipelineInfo->pPSO.Get());
	context.pCmdList->SetGraphicsRootSignature(pipelineInfo->rootSig.GetPtr());
	context.pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	auto vertexBufferView = m_VertexBuffer[frame].GetView();
	context.pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);

	D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = m_ConstantBuffer.GetHandleGPU(frame);
	if (cbHandle.ptr == 0)
	{
		ELOG("Error : ConstantBuffer handle is invalid");
		return;
	}
	context.pCmdList->SetGraphicsRootDescriptorTable(0, cbHandle);
	context.pCmdList->SetGraphicsRootDescriptorTable(1, textureHandle);
	context.pCmdList->DrawInstanced(4, 1, 0, 0);
}

//----------------------------------------------------------------------
//		頂点バッファの作成
//----------------------------------------------------------------------
void Sprite::CreateVertexBuffer()
{
	struct Vertex
	{
		XMFLOAT2 Position;
		XMFLOAT2 TexCoord;
	};

	Vertex vertices[4] = {
	    {{-1.0f, 1.0f}, {m_UVMin.x, m_UVMax.y}},  // 左上
	    {{1.0f, 1.0f}, {m_UVMax.x, m_UVMax.y}},   // 右上
	    {{-1.0f, -1.0f}, {m_UVMin.x, m_UVMin.y}}, // 左下
	    {{1.0f, -1.0f}, {m_UVMax.x, m_UVMin.y}}   // 右下
	};

	auto device = GetDevice();
	if (!device)
		return;

	for (auto i = 0u; i < kFrameCount; ++i)
	{
		if (!m_VertexBuffer[i].Init<Vertex>(device.Get(), 4, vertices))
			ELOG("Error : VertexBuffer::Init() Failed.");
	}
}

void Sprite::SetUV(const XMFLOAT2& uvMin, const XMFLOAT2& uvMax)
{
	m_UVMin = uvMin;
	m_UVMax = uvMax;
	// 頂点へのUV反映は Draw() で現在のフレームスロットに対して行う
}

//----------------------------------------------------------------------
//		定数バッファの更新
//----------------------------------------------------------------------
void Sprite::UpdateConstantBuffers(uint32_t frameIndex)
{
	XMFLOAT2 screenSize = {SCREEN_WIDTH, SCREEN_HEIGHT};

	SpriteBuffer* pBuffer = m_ConstantBuffer.GetPtr<SpriteBuffer>(frameIndex);
	if (!pBuffer)
	{
		ELOG("Error : ConstantBuffer::GetPtr() returned nullptr");
		return;
	}

	pBuffer->Position_Padding        = XMFLOAT4(m_Position.x, m_Position.y, 0.0f, 0.0f);
	pBuffer->Size_Padding            = XMFLOAT4(m_Size.x, m_Size.y, 0.0f, 0.0f);
	pBuffer->Scale_Padding           = XMFLOAT4(m_Scale.x, m_Scale.y, 0.0f, 0.0f);
	pBuffer->Rotation_Anchor_Padding = XMFLOAT4(m_Rotation, 0.0f, 0.0f, 0.0f);
	pBuffer->ScreenSize_Padding      = XMFLOAT4(screenSize.x, screenSize.y, 0.0f, 0.0f);
	pBuffer->Color                   = m_Color;
}
