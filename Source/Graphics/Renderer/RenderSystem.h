#pragma once
#include "PipelineStateManager.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "GameObject.h"
#include "RenderContext.h"
#include <d3d12.h>
#include <ComPtr.h>
#include <string>
#include <map>
#include "main.h"

/// <summary>
/// DirectX12を用いた描画処理を管理するシステムクラス。
/// パイプライン設定やモデル描画のインターフェースを提供します。
/// </summary>
class RenderSystem
{
public:
	RenderSystem();
	~RenderSystem();

	// 各描画マネージャーの作成と初期設定
	bool Init();

	// 保持している各マネージャーの解放
	void Term();

	// 特定のモデルパスに対して使用するパイプラインを紐づけます。
	[[nodiscard]] bool SetupModelPipeline(const std::wstring& modelPath, const std::wstring& pipelineName);

	// ワールド行列を指定してモデルを描画します。
	[[nodiscard]] bool DrawModel(const RenderContext& context, const std::wstring& modelPath, const XMMATRIX& worldMatrix, int materialId = 0);

	// モデル内の全メッシュを描画します。
	[[nodiscard]] bool DrawModelAllMeshes(
		const RenderContext& context,
		const std::wstring& modelPath,
		const XMMATRIX& worldMatrix,
		D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE boneMatrixCBHandle = {});

	// メッシュを描画
	[[nodiscard]] bool DrawMesh(
		const RenderContext& context,
		Mesh* mesh,
		Material* material,
		const XMMATRIX& worldMatrix,
		int materialId,
		D3D12_GPU_DESCRIPTOR_HANDLE meshCBHandle);

	// マテリアルのテクスチャをバインド
	[[nodiscard]] bool BindMaterialTextures(
		ID3D12GraphicsCommandList* pCmdList,
		Material* material,
		int materialId);

	// パイプラインステートを設定
	[[nodiscard]] bool SetPipelineState(
		const RenderContext& context,
		const std::wstring& pipelineName);

	// モデルにパイプラインが設定されているか確認
	[[nodiscard]] bool IsModelPipelineSetup(const std::wstring& modelPath) const;

private:
	ComPtr<ID3D12Device>		m_Device;
	DescriptorPool*				m_Pool[POOL_COUNT] = { nullptr };
	ComPtr<ID3D12CommandQueue>	m_Queue;

	PipelineStateManager*		m_PipelineManager = nullptr;
	ShaderManager*				m_ShaderManager = nullptr;
	ModelManager*				m_ModelManager = nullptr;

	std::map<std::wstring, std::wstring> m_ModelPipelineMap;

	RenderSystem(const RenderSystem&) = delete;
	void operator=(const RenderSystem&) = delete;
};