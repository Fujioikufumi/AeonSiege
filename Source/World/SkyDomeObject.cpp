#include "SkyDomeObject.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "NameSpace.h"
#include "FileUtil.h"
#include "ResourceUploadBatch.h"
#include "main.h"
#include "Scene.h"
#include "GameManager.h"
#include "Camera.h"

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
SkyDomeObject::SkyDomeObject()
	: GameObject()
	, m_TexturePath(L"Assets/Texture/Sky/4kHDR.dds")
	, m_Radius(100.0f)
	, m_Slices(32)
	, m_Stacks(16)
{
}

//-----------------------------------------------------------------------------
// 	    デストラクタ
//-----------------------------------------------------------------------------
SkyDomeObject::~SkyDomeObject()
{
	Term();
}

//-----------------------------------------------------------------------------
//      初期化処理
//-----------------------------------------------------------------------------
bool SkyDomeObject::Init()
{
	DLOG("SkyDomeObject::Init() called");

	auto device = ResourceManager::GetInstance().GetDevice();
	auto pool = ResourceManager::GetInstance().GetPool(POOL_TYPE_RES);
	auto queue = ResourceManager::GetInstance().GetQueue();

	if (!device || !pool || !queue)
	{
		ELOG("Error : ResourceManager not initialized");
		return false;
	}

	// テクスチャ読み込み
	std::wstring fullPath;
	if (!SearchFilePath(m_TexturePath.c_str(), fullPath))
	{
		ELOG("Error : SkyDome texture file not found: %ls", m_TexturePath.c_str());
		return false;
	}

	m_Texture = std::make_unique<Texture>();
	DirectX::ResourceUploadBatch batch(device.Get());
	batch.Begin();

	if (!m_Texture->Init(device.Get(), pool, fullPath.c_str(), false, batch))
	{
		ELOG("Error : SkyDome Texture::Init() Failed. path = %ls", fullPath.c_str());
		return false;
	}

	auto finish = batch.End(queue.Get());
	finish.wait();

	// スカイドーム初期化
	if (!m_SkyDome.Init(
		device.Get(),
		pool,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_D32_FLOAT,
		m_Radius,
		m_Slices,
		m_Stacks))
	{
		ELOG("Error : SkyDome::Init() Failed.");
		return false;
	}

	DLOG("SkyDomeObject::Init() - Success");
	return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void SkyDomeObject::Term()
{
	m_SkyDome.Term();
	if (m_Texture)
	{
		m_Texture->Term();
		m_Texture.reset();
	}
}

//-----------------------------------------------------------------------------
//      更新処理
//-----------------------------------------------------------------------------
void SkyDomeObject::Update(float deltaTime)
{
	Scene* scene = GameManager::GetScene();
	Camera* camera = scene->GetCamera();
	m_Position = camera->GetPosition();
	GameObject::Update(deltaTime);
}

//-----------------------------------------------------------------------------
//      描画処理
//-----------------------------------------------------------------------------
void SkyDomeObject::Draw(const RenderContext& context)
{
	if (!m_Texture)
		return;
	XMMATRIX viewMatrix = context.viewMatrix;
	XMMATRIX projMatrix = context.projMatrix;

	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = m_Texture->GetHandleGPU();
	m_SkyDome.Draw(context.pCmdList, textureHandle, viewMatrix, projMatrix);
}

D3D12_GPU_DESCRIPTOR_HANDLE SkyDomeObject::GetTextureHandle() const
{
	if (m_Texture)
	{
		return m_Texture->GetHandleGPU();
	}
	return { 0 };
}
