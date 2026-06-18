#pragma once
#include <vector>
#include <d3d12.h>

enum class RootParamIndex : UINT {
	TransformCB = 0,	// b0
	LightCB = 1,		// b1
	CameraCB = 2,		// b2
	MaterialCB = 3,		// b4 (RootSignature上は3番目のディスクリプタ)

	// テクスチャ (通常のマテリアル用)
	BaseColorMap = 4,	// t0
	MetallicMap = 5,	// t1
	RoughnessMap = 6,	// t2
	NormalMap = 7,		// t3

	// 地形専用
	FieldMap = 8,		// t4
	GrassTexture = 9,	// t5
	RockTexture = 10,	// t6
	MacroTexture = 11,	// t10
};

//======================================================================
// 		パイプライン初期化クラス
//======================================================================
class PipelineInitializer
{
public:
	/// 全てのパイプラインを初期化
	static bool InitializeAllPipelines();
private:
	//-----------------------------------------------------------------------------
	// 	パイプライン初期化

	/// モデルパイプライン初期化
	static bool InitializeModelPipeline();

	/// スキニングFBXパイプライン初期化
	static bool InitializeSkinnedFBXPipeline();

	/// UIパイプラインの初期化
	static bool InitializeUIPipeline();

	/// シャドウパイプラインの初期化
	static bool InitializeShadowPipeline();

	/// 地形パイプラインの初期化
	static bool InitalizeTerrainPipeline();

	/// 草原パイプラインの初期化(ジオメトリシェーダー)
	static bool InitalizeMeadowGSPipeline();

	// 草原パイプラインの初期化(カリング用)
	static bool InitializeGrassGeneratorPipeline();

	/// マスクドUIパイプラインの初期化
	static bool InitalizeMaskedUIPipeline();

private:
	//-----------------------------------------------------------------------------
	// 	インプットレイアウト取得

	/// 共通モデル用インプットレイアウトを取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetModelInputLayout();

	/// スキニングモデル用インプットレイアウトを取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetSkinnedModelInputLayout();

	/// UI用インプットレイアウトを取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetUIInputLayout();

	/// 地形用インプットレイアウトを取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetTerrainInputLayout();

	/// インスタンシング用インプットレイアウト（メッシュ頂点＋インスタンスWorld）を取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInstancedModelInputLayout();

	/// 草原描画用ジオメトリシェーダーパイプラインのインプットレイアウトを取得
	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetMeadowGSInputLayout();
};