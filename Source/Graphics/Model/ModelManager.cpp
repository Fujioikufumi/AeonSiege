#include "ModelManager.h"
#include "Logger.h"
#include "FileUtil.h"
#include <algorithm>
#include "ResourceManager.h"
#include "main.h"
#include "AnimationManager.h"
#include "MaterialResolver.h"
#include "BinaryModelLoader.h"
#include "ConstantBuffer.h"

//-----------------------------------------------------------------------------
//	  初期化処理
//-----------------------------------------------------------------------------
bool ModelManager::Init()
{
	// リソースを取得
	auto& resourceManager = ResourceManager::GetInstance();

	// デバイス取得
	m_Device = resourceManager.GetDevice();

	// キュー取得
	m_Queue = resourceManager.GetQueue();

	// プール取得
	for (int i = 0; i < POOL_COUNT; ++i)
	{
		m_Pool[i] = resourceManager.GetPool(static_cast<POOL_TYPE>(i));
	}
	
	if (!m_Device || !m_Queue)
	{
		ELOG("Error: ResourceManager not initialized");
		return false;
	}
	for (int i = 0; i < POOL_COUNT; ++i)
	{
		if (m_Pool[i] == nullptr)
		{
			ELOG("Error : DescriptorPool[%d] is null", i);
			return false;
		}
	}

	return true;
}

void ModelManager::Term()
{
	UnloadAll();
	m_Device.Reset();
	m_Queue.Reset();
	for (int i = 0; i < POOL_COUNT; ++i)
	{
		m_Pool[i] = nullptr;
	}
	DLOG("ModelManager terminated");
}

//-----------------------------------------------------------------------------
//      モデルのロード
//-----------------------------------------------------------------------------
bool ModelManager::LoadModel(const std::wstring& modelPath)
{
	// ファイルパスが既にロードされているか確認
	auto it = m_LoadedModels.find(modelPath);
	if (it != m_LoadedModels.end())
	{
		// 既にロードされていたら参照カウントを増やして終了
		it->second->referenceCount++;
		return true;
	}

	// unique_ptrを使用
	auto modelInfo = std::make_unique<ModelInfo>();
	modelInfo->modelPath = modelPath;
	modelInfo->referenceCount = 1;

	// モデルの内部ロード処理を呼び出す
	if (!LoadModelInternal(modelPath, *modelInfo))
	{
		ELOG("Failed to load model: %ls", modelPath.c_str());
		return false;
	}

	// ロードしたモデル情報を保存
	m_LoadedModels[modelPath] = std::move(modelInfo);

	return true;
}

//-----------------------------------------------------------------------------
// 	モデルの非同期ロード
//-----------------------------------------------------------------------------
void ModelManager::LoadModelAsync(const std::wstring& modelPath)
{
	if (CheckModelLoaded(modelPath))
	{
		return;
	}

	auto modelInfo = std::make_unique<ModelInfo>();
	modelInfo->modelPath = modelPath;
	modelInfo->referenceCount = 1;
	modelInfo->isLoaded = false;
	m_LoadedModels[modelPath] = std::move(modelInfo);

	// 別スレッドで解析処理を開始
	std::thread([this, modelPath]() {
		ModelLoadData data = ParseModelData(modelPath);

		// 解析が終わったらキューに追加
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_LoadedDataQueue.push(std::move(data));
		}).detach();
}

//-----------------------------------------------------------------------------
// 	解析が終わったモデルのGPUリソースを生成
//-----------------------------------------------------------------------------
void ModelManager::ProcessLoadedModels()
{
	// キューから解析済みデータを取り出す
	std::lock_guard<std::mutex> lock(m_QueueMutex);

	while (!m_LoadedDataQueue.empty())
	{
		ModelLoadData data = std::move(m_LoadedDataQueue.front());
		m_LoadedDataQueue.pop();
		auto it = m_LoadedModels.find(data.modelPath);
		if (it == m_LoadedModels.end()) continue;
		if (!data.loadSuccess) {
			ELOG("Async load failed for: %ls", data.modelPath.c_str());
			continue;
		}
		// GPUリソースの生成
		if (FinalizeModel(data, *it->second)) {
			it->second->isLoaded = true;
		}
	}
}

//-----------------------------------------------------------------------------
// 		CPUでのファイル解析処理 (別スレッドで実行)
//-----------------------------------------------------------------------------
ModelLoadData ModelManager::ParseModelData(const std::wstring& modelPath)
{
	ModelLoadData data;
	data.modelPath = modelPath;
	data.loadSuccess = false;

	std::wstring fullPath;
	if (!SearchFilePath(modelPath.c_str(), fullPath))
	{
		// ファイルが見つからない場合はエラーログを出して、loadSuccessをfalseのままで返す。
		ELOG("Model file not found: %ls", modelPath.c_str());
		return data;
	}
	data.modelDirectory = GetDirectoryPath(fullPath.c_str());
	std::unique_ptr<IModelLoader> loader = CreateModelLoader(fullPath);
	if (!loader) return data;

	data.loadSuccess = loader->Load(fullPath.c_str(), data.resMeshes, data.resMaterials, &data.skeleton, nullptr);
	return data;
}

bool ModelManager::FinalizeModel(ModelLoadData& loadData, ModelInfo& modelInfo)
{
	if (!loadData.skeleton.bones.empty()) {
		modelInfo.skeleton = loadData.skeleton;
		modelInfo.hasAnimation = true;
		AnimationManager::GetInstance().RegisterSkeleton(loadData.modelPath, loadData.skeleton);
	}
	else {
		modelInfo.hasAnimation = false;
	}
	modelInfo.modelType = DetermineModelType(loadData.resMeshes, modelInfo.hasAnimation, modelInfo.skeleton);
	bool hasSkinnedMesh = false;
	for (const auto& mesh : loadData.resMeshes) {
		if (mesh.HasAnimation) { hasSkinnedMesh = true; break; }
	}
	if (hasSkinnedMesh && !InitializeDefaultBoneMatrixCB(modelInfo, hasSkinnedMesh)) return false;

	// DX12リソース生成
	if (!CreateMeshes(loadData.resMeshes, modelInfo.meshes)) return false;

	if (!MaterialResolver::Build(
		m_Device.Get(), m_Pool[POOL_TYPE_RES], m_Queue.Get(),
		loadData.modelDirectory, loadData.modelPath, loadData.resMaterials, modelInfo.material)) {
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
//      モデルの内部ロード処理
//-----------------------------------------------------------------------------
bool ModelManager::LoadModelInternal(const std::wstring& modelPath, ModelInfo& modelInfo)
{
	// 1. ファイルパスの解決
	std::wstring fullPath;
	if (!SearchFilePath(modelPath.c_str(), fullPath))
	{
		ELOG("Model file not found: %ls", modelPath.c_str());
		return false;
	}

	modelInfo.modelPath = fullPath;
	modelInfo.modelDirectory = GetDirectoryPath(fullPath.c_str());

	// 2. FBXならFBXLoaderを使用し、それ以外ならAssimpLoaderを使用
	std::unique_ptr<IModelLoader> loader = CreateModelLoader(fullPath);
	if (loader == nullptr)
	{
		ELOG("Failed to create model loader for: %ls", fullPath.c_str());
		return false;
	}

	// 3. メッシュとマテリアルの読み込み
	std::vector<ResMesh> resMesh;
	std::vector<ResMaterial> resMaterial;
	SkeletonInfo skeleton;

	if (!loader->Load(fullPath.c_str(), resMesh, resMaterial, &skeleton, nullptr))
	{
		ELOG("Failed to load model using %s: %ls", loader->GetLoaderName(), fullPath.c_str());
		return false;
	}

	// 4. スケルトン情報の処理（FBXの場合のみ）
	if (!skeleton.bones.empty())
	{
		modelInfo.skeleton = skeleton;
		modelInfo.hasAnimation = true;        // スケルトンがある = アニメーション可能
		modelInfo.animations = std::nullopt;  // モデルファイルにはアニメーションを含まない

		// AnimationManagerにスケルトンのみ登録
		AnimationManager::GetInstance().RegisterSkeleton(modelPath, skeleton);
		AnimationManager::GetInstance().RegisterAnimations(modelPath, std::vector<AnimationClip>());
	}
	else
	{
		modelInfo.hasAnimation = false;
	}

	// 5. モデルタイプの判定
	modelInfo.modelType = DetermineModelType(resMesh, modelInfo.hasAnimation, modelInfo.skeleton);

	// 6. スキニングメッシュの確認
	bool hasSkinnedMesh = false;
	for (const auto& mesh : resMesh)
	{
		if (mesh.HasAnimation)
		{
			hasSkinnedMesh = true;
			break;
		}
	}

	// 7. デフォルトボーンマトリックスCBVの初期化
	if (hasSkinnedMesh && !InitializeDefaultBoneMatrixCB(modelInfo, hasSkinnedMesh))
	{
		ELOG("Failed to initialize default bone matrix CB");
		return false;
	}

	// 8. メッシュの生成
	if (!CreateMeshes(resMesh, modelInfo.meshes))
	{
		ELOG("Failed to create meshes");
		return false;
	}

	// 9. マテリアルの生成 + テクスチャのロード(MaterialResolver)
	if (!MaterialResolver::Build(
		m_Device.Get(),
		m_Pool[POOL_TYPE_RES],
		m_Queue.Get(),
		modelInfo.modelDirectory,
		modelInfo.modelPath,
		resMaterial,
		modelInfo.material))
	{
		ELOG("Failed to build material via MaterialResolver");
		return false;
	}

	modelInfo.isLoaded = true;
	return true;
}


//-----------------------------------------------------------------------------
//      モデルのアンロード
//-----------------------------------------------------------------------------
void ModelManager::UnloadModel(const std::wstring& modelPath)
{
	// モデルがロードされているか確認
	auto it = m_LoadedModels.find(modelPath);
	if (it == m_LoadedModels.end())
	{
		return; // ロードされていない場合は何もしない
	}

	// 参照カウントをデクリメント
	it->second->referenceCount--;

	if (it->second->referenceCount <= 0)
	{
		// メッシュを削除
		for (auto& mesh : it->second->meshes)
		{
			if (mesh != nullptr)
			{
				delete mesh;
			}
		}

		// マテリアルを終了
		it->second->material.Term();

		// デフォルトボーンマトリックスCBVを終了（optionalの場合のみ）
		if (it->second->m_DefaultBoneMatrixCB.has_value())
		{
			it->second->m_DefaultBoneMatrixCB.value().Term();
		}
		// アニメーション管理側の登録も外す (参照はLoadModel時に登録したパス)
		AnimationManager::GetInstance().UnregisterModel(modelPath);
		// マップから削除
		m_LoadedModels.erase(it);
	}
}

//-----------------------------------------------------------------------------
//	  全モデルのアンロード
//-----------------------------------------------------------------------------
void ModelManager::UnloadAll()
{
	for (auto& pair : m_LoadedModels)
	{
		for (auto& mesh : pair.second->meshes)
		{
			// 各モデルと同じキーで AnimationManager を削除する
			AnimationManager::GetInstance().UnregisterModel(pair.first);

			// メッシュを削除
			if (mesh != nullptr)
			{
				delete mesh;
			}
		}

		// マテリアルを終了
		pair.second->material.Term();

		// デフォルトボーンマトリックスCBVを終了（optionalの場合のみ実行）
		if (pair.second->m_DefaultBoneMatrixCB.has_value())
		{
			pair.second->m_DefaultBoneMatrixCB.value().Term();
		}
	}

	// マップをクリア
	m_LoadedModels.clear();
	m_LoadedMaterials.clear();
	m_LoadedTextures.clear();

	DLOG("All models unloaded");
}

//-----------------------------------------------------------------------------
//      モデル取得
//-----------------------------------------------------------------------------
ModelInfo* ModelManager::GetModel(const std::wstring& modelPath)
{
	auto it = m_LoadedModels.find(modelPath);
	if (it != m_LoadedModels.end())
	{
		// モデル情報を返す
		return it->second.get();
	}
	
	// 見つからない場合はnullptrを返す
	return nullptr;
}

//-----------------------------------------------------------------------------
//      モデル取得
//-----------------------------------------------------------------------------
std::vector<Mesh*> ModelManager::GetModelMeshes(const std::wstring& modelPath)
{
	auto modelInfo = GetModel(modelPath);
	if (modelInfo != nullptr)
	{
		// メッシュの配列を返す
		return modelInfo->meshes;
	}
	// 空の配列をを返す
	return std::vector<Mesh*>();
}

//-----------------------------------------------------------------------------
//	  モデルのマテリアル取得
//-----------------------------------------------------------------------------
Material* ModelManager::GetModelMaterial(const std::wstring& modelPath)
{
	auto modelInfo = GetModel(modelPath);
	if (modelInfo != nullptr)
	{
		// マテリアルを返す
		return &modelInfo->material;
	}
	// 見つからない場合はnullptrを返す
	return nullptr;
}

//-----------------------------------------------------------------------------
//	  参照カウント管理
//-----------------------------------------------------------------------------
void ModelManager::AddReference(const std::wstring& modelPath)
{
	auto it = m_LoadedModels.find(modelPath);
	if (it != m_LoadedModels.end())
	{
		it->second->referenceCount++;
	}
}

//-----------------------------------------------------------------------------
//	  指定したモデルを削除
//-----------------------------------------------------------------------------
void ModelManager::RemoveReference(const std::wstring& modelPath)
{
	UnloadModel(modelPath);
}

//-----------------------------------------------------------------------------
//	  参照カウント取得
//-----------------------------------------------------------------------------
uint32_t ModelManager::GetReferenceCount(const std::wstring& modelPath)
{
	auto it = m_LoadedModels.find(modelPath);
	if (it != m_LoadedModels.end())
	{
		return it->second->referenceCount;
	}
	return 0;
}

//-----------------------------------------------------------------------------
//	  総メモリ使用量取得
//-----------------------------------------------------------------------------
size_t ModelManager::GetTotalMemoryUsage() const
{
	size_t totalMemory = 0;

	for (const auto& pair : m_LoadedModels)
	{
		// メッシュのメモリ使用量を計算
		for (const auto& mesh : pair.second->meshes)
		{
			if (mesh != nullptr)
			{
				// メッシュのメモリ使用量を取得（実装に依存）
				//totalMemory += mesh->GetMemoryUsage();
			}
		}
	}

	return totalMemory;
}

bool ModelManager::CheckModelLoaded(const std::wstring& modelPath)
{
	if (m_LoadedModels.find(modelPath) != m_LoadedModels.end())
	{
		// 既にロードされている場合は参照カウントを増やすだけ
		m_LoadedModels[modelPath]->referenceCount++;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//      ファイル拡張子から適切なローダーを選択
//-----------------------------------------------------------------------------
std::unique_ptr<IModelLoader> ModelManager::CreateModelLoader(const std::wstring& filePath) const
{
	std::wstring extension = GetFileExtension(filePath);
	if (extension == L".bmdl" || extension == L".BMDL")
	{
		return std::make_unique<BinaryModelLoader>();
	}
	ELOG("Unsupported extension: %ls", extension.c_str());
	return nullptr;
}

//-----------------------------------------------------------------------------
//    モデルタイプの判定
//-----------------------------------------------------------------------------
ModelType ModelManager::DetermineModelType(const std::vector<ResMesh>& resMesh, bool hasAnimation, const std::optional<SkeletonInfo>& skeleton) const
{
	bool hasSkinnedMesh = false;
	for (const auto& mesh : resMesh)
	{
		if (mesh.HasAnimation)
		{
			hasSkinnedMesh = true;
			break;
		}
	}

	// モデルタイプを設定
	if (hasAnimation)
	{
		return ModelType::Animated;
	}
	else if (hasSkinnedMesh || (skeleton.has_value() && !skeleton.value().bones.empty()))
	{
		return ModelType::Skinned;
	}
	else
	{
		return ModelType::Static;
	}
}

bool ModelManager::InitializeDefaultBoneMatrixCB(ModelInfo& modelInfo, bool hasSkinnedMesh)
{
	if (!hasSkinnedMesh)
	{
		return true; // スキニングメッシュがない場合は不要
	}
	
	// ボーンマトリックス定数バッファを初期化
	const size_t maxBones = 128; // 最大ボーン数
	struct alignas(maxBones) BoneMatrixBuffer
	{
		XMFLOAT4X4 matrices[maxBones];
	};

	if (!modelInfo.m_DefaultBoneMatrixCB.has_value())
	{
		// std::optionalに定数バッファを作成
		modelInfo.m_DefaultBoneMatrixCB.emplace();
		if (!modelInfo.m_DefaultBoneMatrixCB.value().Init(
			m_Device.Get(),
			m_Pool[POOL_TYPE_RES],
			sizeof(BoneMatrixBuffer)
		))
		{
			ELOG("Failed to initialize default bone matrix constant buffer");
			return false;
		}
	}

	// バインドポーズのボーンマトリックスを計算
	BoneMatrixBuffer buffer = {};

	// スケルトン情報がある場合、Tポーズ用の BonePalette を作る
	if (modelInfo.skeleton.has_value() && !modelInfo.skeleton.value().bones.empty())
	{
		const auto& skeletonRef = modelInfo.skeleton.value();

		// 余りは単位行列で埋める（シェーダ側の安全）
		for (size_t i = 0; i < maxBones; ++i)
		{
			XMStoreFloat4x4(&buffer.matrices[i], XMMatrixIdentity());
		}

		const size_t count = min(skeletonRef.bones.size(), maxBones);

		for (size_t i = 0; i < count; ++i)
		{
			const BoneInfo& b = skeletonRef.bones[i];

			const XMMATRIX palette = b.boneBindGlobal * b.boneBindInv;

			// HLSL側が mul(Matrix, vector) の流儀なので、CPUでTransposeして詰める
			XMStoreFloat4x4(&buffer.matrices[i], XMMatrixTranspose(palette));
		}
	}
	else
	{
		// スケルトン情報がない場合は単位行列で初期化
		for (size_t i = 0; i < maxBones; ++i)
		{
			XMStoreFloat4x4(&buffer.matrices[i], XMMatrixIdentity());
		}
	}

	// バッファにコピー
	void* ptr = modelInfo.m_DefaultBoneMatrixCB.value().GetPtr();
	if (ptr != nullptr)
	{
		memcpy(ptr, &buffer, sizeof(BoneMatrixBuffer));
	}

	return true;
}

//-----------------------------------------------------------------------------
//   メッシュの生成
//-----------------------------------------------------------------------------
bool ModelManager::CreateMeshes(const std::vector<ResMesh>& resMesh, std::vector<Mesh*>& outMeshes)
{
	outMeshes.reserve(resMesh.size());

	for (size_t i = 0; i < resMesh.size(); ++i)
	{
		// メッシュオブジェクトを生成
		auto mesh = new (std::nothrow) Mesh();
		if (mesh == nullptr)
		{
			ELOG("Out of memory");
			// 既に作成したメッシュを削除
			for (auto* m : outMeshes)
			{
				delete m;
			}
			outMeshes.clear();
			return false;
		}

		bool success = false;
		if (resMesh[i].HasAnimation)
		{
			// スキニングメッシュとして初期化
			success = mesh->InitSkinned(m_Device.Get(), resMesh[i]);
		}
		else
		{
			// 通常メッシュとして初期化
			success = mesh->Init(m_Device.Get(), resMesh[i]);
		}

		if (!success)
		{
			ELOG("Mesh Init Failed");
			delete mesh;

			for (auto m : outMeshes)
			{
				delete m;
			}
			outMeshes.clear();
			return false;
		}

		outMeshes.push_back(mesh);
	}

	return true;
}