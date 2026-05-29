#pragma once
#include "SingletonManager.h"
#include "RootSignature.h"
#include "DescriptorPool.h"
#include <d3d12.h>
#include <string>
#include <map>
#include <vector>

//----------------------------------------------------------------------
// 		パイプラインステート情報構造体
//----------------------------------------------------------------------
struct PipelineStateInfo
{
	ComPtr<ID3D12PipelineState> pPSO;	// パイプラインステートオブジェクト
	RootSignature rootSig;				// ルートシグネチャ
	std::wstring shaderName;			// 使用するシェーダー名
	std::wstring pipelineName;			// パイプライン名
	bool isValid;						// 有効かどうか
};

//----------------------------------------------------------------------
// 		パイプラインステート記述子
//----------------------------------------------------------------------
struct PipelineStateDesc
{
	std::wstring shaderName;							// 使用するシェーダー名
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;	// 入力レイアウト
	DXGI_FORMAT rtvFormat;								// レンダーターゲットのフォーマット
	DXGI_FORMAT dsvFormat;								// 深度ステンシルフォーマット	
	D3D12_RASTERIZER_DESC rasterizerState;				// ラスタライザーステート
	D3D12_BLEND_DESC blendState;						// ブレンドステート
	D3D12_DEPTH_STENCIL_DESC depthStencilState;			// 深度ステンシルステート
	UINT sampleCount;									// サンプル数
	UINT sampleQuality;									// サンプル品質
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;	// プリミティブトポロジタイプ
};

//----------------------------------------------------------------------
// 		ルートシグネチャ設定構造体
//----------------------------------------------------------------------
struct RootSignatureConfig
{
	// CBV設定
	struct CBVConfig
	{
		ShaderStage stage;
		int index;
		uint32_t reg;
	};

	// SRV設定
	struct SRVConfig
	{
		ShaderStage stage;
		int index;
		uint32_t reg;
	};

	// UAV設定
	struct UAVConfig
	{
		ShaderStage stage;
		int index;
		uint32_t reg;
	};

	// サンプラー設定
	struct SamplerConfig
	{
		ShaderStage stage;
		uint32_t reg;
		SamplerState state;
	};

	// シャドウサンプラー設定
	struct ShadowSamplerConfig
	{
		ShaderStage stage;
		uint32_t reg;
	};

	std::vector<CBVConfig> cbvConfigs;			// CBV設定
	std::vector<SRVConfig> srvConfigs;			// SRV設定
	std::vector<UAVConfig> uavConfigs;			// UAV設定
	std::vector<SamplerConfig> samplerConfigs;	// サンプラー設定
	std::vector<ShadowSamplerConfig> shadowSamplerConfigs; // シャドウサンプラー設定
	bool allowIL; // Input Layoutを許可するかどうか
	bool allowSO; // Stream Outputを許可するかどうか
};


/// <summary>
/// パイプラインステートおよびルートシグネチャの生成とキャッシュを管理するクラス。
/// </summary>
class PipelineStateManager : public SingletonManager<PipelineStateManager>
{
private:
	friend class SingletonManager<PipelineStateManager>;

	ComPtr<ID3D12Device> m_Device;								// D3D12デバイス
	DescriptorPool* m_Pool = nullptr;						// ディスクリプタプール
	std::map<std::wstring, PipelineStateInfo> m_PipelineStates; // 生成済みパイプラインキャッシュ

	PipelineStateManager() = default;
	~PipelineStateManager() = default;

	// コピー・代入は制限
	PipelineStateManager(const PipelineStateManager&) = delete;
	void operator=(const PipelineStateManager&) = delete;

	[[nodiscard]] RootSignatureConfig CreateDefaultModelRootSignatureConfig();
	[[nodiscard]] bool CreateRootSignature(const RootSignatureConfig& config, RootSignature& outRootSig);

public:
	/// マネージャーの初期化
	bool Init();

	/// デフォルト設定を用いたパイプラインステートの簡易作成。
	bool CreatePipelineState(
		const std::wstring& pipelineName,
		const std::wstring& shaderName,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
		DXGI_FORMAT rtvFormat,
		DXGI_FORMAT dsvFormat,
		const RootSignatureConfig* rootSigConfig = nullptr);

	/// 詳細なパラメータを指定したパイプラインステートの作成。
	bool CreatePipelineState(
		const std::wstring& pipelineName,
		const PipelineStateDesc& desc,
		const RootSignatureConfig* rootSigConfig = nullptr);

	/// コンピュートパイプラインステートを作成
	bool CreateComputePipelineState(
		const std::wstring& pipelineName,
		const std::wstring& computeShaderName,
		const RootSignatureConfig* rootSigConfig);


	/// パイプラインステート情報を取得
	PipelineStateInfo* GetPipelineState(const std::wstring& pipelineName);

	/// パイプラインステートが作成されているか確認
	bool IsPipelineStateCreated(const std::wstring& pipelinename) const;

	/// パイプラインステートを削除
	void RemovePipelineState(const std::wstring& pipelineName);

	/// 全てのパイプラインステートを削除
	void RemoveAll();

	/// 読み込まれているパイプラインステート数を取得
	size_t GetPipelineStateCount() const { return m_PipelineStates.size(); }

	/// 終了処理
	void Term();
};