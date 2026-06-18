#pragma once
#include "SingletonManager.h"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "Mesh.h"
#include "Material.h"
#include "ResMesh.h"
#include "nameSpace.h"
#include <optional>
#include "IModelLoader.h"
#include "ConstantBuffer.h"
#include "Texture.h"

// 並列処理
#include <mutex>
#include <queue>
#include <future>

//-----------------------------------------------------------------------------
// 非同期読み込み用の中間データ
//-----------------------------------------------------------------------------
struct ModelLoadData
{
	std::wstring modelPath;
	std::wstring modelDirectory;
	std::vector<ResMesh> resMeshes;
	std::vector<ResMaterial> resMaterials;
	SkeletonInfo skeleton;
	bool loadSuccess = false;
};

enum class ModelType
{
	Static,     // 静的メッシュ
	Skinned,    // スキニングメッシュ
	Animated,   // アニメーション付きスキニングメッシュ
};

//-----------------------------------------------------------------------------
// モデル情報
//-----------------------------------------------------------------------------
struct ModelInfo
{
	std::vector<Mesh*> meshes;      // メッシュ
	Material material;              // マテリアル
	std::wstring modelPath;         // モデルパス
	std::wstring modelDirectory;    // モデルディレクトリパス
	uint32_t referenceCount = 0;    // 参照カウント
	bool isLoaded = false;          // ロード済みフラグ

	// モデルタイプ
	ModelType modelType = ModelType::Static;

	// アニメーション関連
	std::optional<SkeletonInfo> skeleton;   // スケルトン情報
	std::optional<std::vector<AnimationClip>> animations;  // アニメーションクリップ
	std::optional<ConstantBuffer> m_DefaultBoneMatrixCB;

	bool hasAnimation = false;              // アニメーションがあるかどうか


	// ヘルパーメソッド
	[[nodiscard]] bool IsSkinned() const {
		return modelType == ModelType::Skinned || modelType == ModelType::Animated;
	}
	[[nodiscard]] bool HasAnimation() const {
		return modelType == ModelType::Animated;
	}

	// アクセサ
	[[nodiscard]] const SkeletonInfo* GetSkeleton() const {
		return skeleton.has_value() ? &skeleton.value() : nullptr;
	}
	[[nodiscard]] SkeletonInfo* GetSkeleton() {
		return skeleton.has_value() ? &skeleton.value() : nullptr;
	}
	[[nodiscard]] const std::vector<AnimationClip>* GetAnimations() const {
		return animations.has_value() ? &animations.value() : nullptr;
	}
	[[nodiscard]] std::vector<AnimationClip>* GetAnimations() {
		return animations.has_value() ? &animations.value() : nullptr;
	}
	[[nodiscard]] ConstantBuffer* GetDefaultBoneMatrixCB() {
		return m_DefaultBoneMatrixCB.has_value() ? &m_DefaultBoneMatrixCB.value() : nullptr;
	}
	[[nodiscard]] const ConstantBuffer* GetDefaultBoneMatrixCB() const {
		return m_DefaultBoneMatrixCB.has_value() ? &m_DefaultBoneMatrixCB.value() : nullptr;
	}
};

//-----------------------------------------------------------------------------
//       モデルマネージャークラス(シングルトン)
//-----------------------------------------------------------------------------
class ModelManager : public SingletonManager<ModelManager>
{
private:
	friend class SingletonManager<ModelManager>;

	// D3D12リソース
	ComPtr<ID3D12Device>		m_Device;
	DescriptorPool*				m_Pool[POOL_COUNT];
	ComPtr<ID3D12CommandQueue>	m_Queue;

	std::map<std::wstring, std::unique_ptr<ModelInfo>>	m_LoadedModels;
	std::map<std::wstring, std::unique_ptr<Material>>	m_LoadedMaterials;
	std::map<std::wstring, std::unique_ptr<Texture>>	m_LoadedTextures;

	ModelManager() = default; // 外部からの作成を禁止
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;      // コピー禁止
	void operator = (const ModelManager&) = delete;  // 代入禁止

public:
	bool Init();
	void Term();

	/// モデル読み込み
	bool LoadModel(const std::wstring& modelPath);

	/// モデル非同期読み込み
	void LoadModelAsync(const std::wstring& modelPath);

	/// 非同期読み込みされたモデルのGPUリソースを生成
	void ProcessLoadedModels();

	/// モデル解放
	void UnloadModel(const std::wstring& modelPath);

	/// 全モデル解放
	void UnloadAll();

	/// モデル情報の取得
	[[nodiscard]] ModelInfo* GetModel(const std::wstring& modelPath);

	/// モデルのメッシュ一覧取得
	[[nodiscard]] std::vector<Mesh*> GetModelMeshes(const std::wstring& modelPath);

	/// モデルのマテリアル取得
	[[nodiscard]] Material* GetModelMaterial(const std::wstring& modelPath);

	/// モデル参照カウント管理
	void AddReference(const std::wstring& modelPath);

	/// モデル参照カウント管理
	void RemoveReference(const std::wstring& modelPath);

	/// モデル参照カウント取得
	[[nodiscard]] uint32_t GetReferenceCount(const std::wstring& modelPath);

	// 統計情報
	[[nodiscard]] size_t GetLoadedModelCount() const { return m_LoadedModels.size(); }
	[[nodiscard]] size_t GetLoadedMaterialCount() const { return m_LoadedMaterials.size(); }
	[[nodiscard]] size_t GetLoadedTextureCount() const { return m_LoadedTextures.size(); }

	/// ロードされている全モデルのパスを取得
	[[nodiscard]] std::vector<std::wstring> GetAllModelPaths() const
	{
		std::vector<std::wstring> paths;
		paths.reserve(m_LoadedModels.size());
		for (const auto& pair : m_LoadedModels)
		{
			paths.push_back(pair.first);
		}
		return paths;
	}

	// メモリ使用量取得
	[[nodiscard]] size_t GetTotalMemoryUsage() const;

private:
	/// モデルが既にロードされているか確認
	bool CheckModelLoaded(const std::wstring& modelPath);

	// CPUでのファイル解析処理 (別スレッド)
	ModelLoadData ParseModelData(const std::wstring& modelPath);

	// GPUリソース生成処理 (メインスレッド)
	bool FinalizeModel(ModelLoadData& loadData, ModelInfo& modelInfo);

	std::queue<ModelLoadData> m_LoadedDataQueue; // CPUで解析されたモデルデータのキュー
	std::mutex m_QueueMutex;                     // キューアクセス用のミューテックス

	// 内部モデル読み込み処理
	bool LoadModelInternal(const std::wstring& modelPath, ModelInfo& modelInfo);

	/// <summary>
	/// ファイル拡張子から適切なローダーを選択
	/// </summary>
	std::unique_ptr<IModelLoader> CreateModelLoader(const std::wstring& filePath) const;

	//=============================================================================
	//      モデルデータ登録メソッド
	//=============================================================================
	/// <summary>
	/// モデルタイプの判定
	/// </summary>
	ModelType DetermineModelType(
		const std::vector<ResMesh>& resMesh,
		bool hasAnimation,
		const std::optional<SkeletonInfo>& skeleton) const;

	/// <summary>
	/// デフォルトボーンマトリックスCBVの初期化と計算
	/// </summary>
	bool InitializeDefaultBoneMatrixCB(
		ModelInfo& modelInfo,
		bool hasSkinnedMesh);

	/// <summary>
	/// メッシュの生成（ResMesh → Meshオブジェクト）
	/// </summary>
	bool CreateMeshes(
		const std::vector<ResMesh>& resMesh,
		std::vector<Mesh*>& outMeshes);
};