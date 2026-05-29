#include "Sprite.h"
#include "ResourceManager.h"
#include "FileUtil.h"
#include "Logger.h"
#include "ResourceUploadBatch.h"
#include "PipelineStateManager.h"
#include <DirectXHelpers.h>
#include "NameSpace.h"
//----------------------------------------------------------------------
//		2Dスプライトコンポーネント
//----------------------------------------------------------------------
Sprite::Sprite(GameObject* obj)
	: Component(obj)
	, m_Position({ 0.0f, 0.0f })
	, m_Size({ 100.0f, 100.0f })
	, m_Scale({ 1.0f, 1.0f })
	, m_Rotation(0.0f)
	, m_UVMin({ 0.0f, 0.0f })
	, m_UVMax({ 1.0f, 1.0f })
	, m_Color({ 1.0f, 1.0f, 1.0f, 1.0f })
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
	Term(); // 既存のリソースを解放してから初期化
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
	auto pool = GetPool(POOL_TYPE_RES);
	auto queue = GetQueue();

	if (!device || !pool || !queue)
	{
		ELOG("Error : ResourceManager not initialized");
		return false;
	}

	// テクスチャの作成
	m_Texture = std::make_unique<Texture>();
	DirectX::ResourceUploadBatch batch(device.Get());
	batch.Begin();
	if (!m_Texture->Init(device.Get(), pool, fullPath.c_str(), true, batch))
	{
		ELOG("Error : Texture::Init() Failed. path = %ls", fullPath.c_str());
		return false;
	}
	auto finish = batch.End(queue.Get());
	finish.wait();

	// 頂点バッファの作成
	CreateVertexBuffer();

	// 定数バッファの作成
	if (!m_ConstantBuffer.Init(device.Get(), pool, sizeof(SpriteBuffer)))
	{
		ELOG("Error : ConstantBuffer::Init() Failed.");
		return false;
	}

	// 一度だけ更新処理を実行する。
	UpdateConstantBuffer();

	// スプライトの初期状態をログに出力
	SpriteBuffer* pBuffer = m_ConstantBuffer.GetPtr<SpriteBuffer>();
	if (pBuffer)
	{
		DLOG("ConstantBuffer after Init and UpdateConstantBuffer:");
		DLOG("  Size_Padding = (%f, %f, %f, %f)",
			pBuffer->Size_Padding.x, pBuffer->Size_Padding.y,
			pBuffer->Size_Padding.z, pBuffer->Size_Padding.w);
	}

	return true;
}

//----------------------------------------------------------------------
//		終了処理
//----------------------------------------------------------------------
void Sprite::Term()
{
	m_ConstantBuffer.Term();
	m_VertexBuffer.Term();

	if (m_Texture)
	{
		m_Texture->Term();
		m_Texture.reset();
	}
}

//----------------------------------------------------------------------
//		更新処理
//----------------------------------------------------------------------
void Sprite::Update(float deltaTime)
{
	UpdateConstantBuffer();
}

//----------------------------------------------------------------------
//		描画処理
//----------------------------------------------------------------------
void Sprite::Draw(const RenderContext& context)
{
	if (!m_Texture || !context.pCmdList)
		return;

	// UIパイプラインの取得
	auto* pipelineInfo = PipelineStateManager::GetInstance().GetPipelineState(L"UIPipeline");

	if (!pipelineInfo || !pipelineInfo->isValid)
	{
		ELOG("Error : UiPipeline not found or invalid");
		return;
	}

	// テクスチャのGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = m_Texture->GetHandleGPU();
	if (textureHandle.ptr == 0)
	{
		ELOG("Error : Texture handle is invalid");
		return;
	}

	// パイプラインステートとルートシグネチャの設定
	context.pCmdList->SetPipelineState(pipelineInfo->pPSO.Get());
	context.pCmdList->SetGraphicsRootSignature(pipelineInfo->rootSig.GetPtr());
	context.pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 頂点バッファの設定
	auto vertexBufferView = m_VertexBuffer.GetView();
	context.pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);

	D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = m_ConstantBuffer.GetHandleGPU();
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
	// 頂点構造体
	struct Vertex
	{
		XMFLOAT2 Position;
		XMFLOAT2 TexCoord;
	};

	// 頂点データの定義
	Vertex vertices[4] = {
		{ { -1.0f,  1.0f }, {m_UVMin.x, m_UVMax.y}}, // 左上
		{ {  1.0f,  1.0f }, {m_UVMax.x, m_UVMax.y}}, // 右上
		{ { -1.0f, -1.0f }, {m_UVMin.x, m_UVMin.y}}, // 左下
		{ {  1.0f, -1.0f }, {m_UVMax.x, m_UVMin.y}}  // 右下
	};

	auto device = GetDevice();
	if (!device)
		return;

	if (!m_VertexBuffer.Init<Vertex>(device.Get(), 4, vertices))
	{
		ELOG("Error : VertexBuffer::Init() Failed.");
	}
}

void Sprite::SetUV(const XMFLOAT2& uvMin, const XMFLOAT2& uvMax)
{
	m_UVMin = uvMin;
	m_UVMax = uvMax;

	if (m_VertexBuffer.GetView().SizeInBytes == 0)
		return;
	// 頂点構造体 (CreateVertexBuffer内の定義と同じもの)
	struct Vertex
	{
		XMFLOAT2 Position;
		XMFLOAT2 TexCoord;
	};
	Vertex* vertices = m_VertexBuffer.Map<Vertex>();
	if (vertices)
	{
		// 頂点のUV座標だけを書き換える (Positionはシェーダーで計算されるためそのまま)
		vertices[0].TexCoord = { m_UVMin.x, m_UVMax.y }; // 左上
		vertices[1].TexCoord = { m_UVMax.x, m_UVMax.y }; // 右上
		vertices[2].TexCoord = { m_UVMin.x, m_UVMin.y }; // 左下
		vertices[3].TexCoord = { m_UVMax.x, m_UVMin.y }; // 右下
		// 参考コードの Unmap() と同じ
		m_VertexBuffer.Unmap();
	}
}

//----------------------------------------------------------------------
//		定数バッファの更新
//----------------------------------------------------------------------
void Sprite::UpdateConstantBuffer()
{
	XMFLOAT2 screenSize = { SCREEN_WIDTH, SCREEN_HEIGHT };

	SpriteBuffer* pBuffer = m_ConstantBuffer.GetPtr<SpriteBuffer>();
	if (!pBuffer)
	{
		ELOG("Error : ConstantBuffer::GetPtr() returned nullptr");
		return;
	}

	pBuffer->Position_Padding = XMFLOAT4(m_Position.x, m_Position.y, 0.0f, 0.0f);
	pBuffer->Size_Padding = XMFLOAT4(m_Size.x, m_Size.y, 0.0f, 0.0f);
	pBuffer->Scale_Padding = XMFLOAT4(m_Scale.x, m_Scale.y, 0.0f, 0.0f);
	pBuffer->Rotation_Anchor_Padding = XMFLOAT4(m_Rotation, 0.0f, 0.0f, 0.0f);
	pBuffer->ScreenSize_Padding = XMFLOAT4(screenSize.x, screenSize.y, 0.0f, 0.0f);
	pBuffer->Color = m_Color;
}
