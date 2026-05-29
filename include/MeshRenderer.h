#pragma once
#include "Component.h"
#include "ModelManager.h"
#include <string>
#include "RenderSystem.h"
#include "ConstantBuffer.h"

/// <summary>
/// 3Dモデルの描画を制御するコンポーネント。
/// </summary>
class MeshRenderer : public Component
{
public:
	explicit MeshRenderer(GameObject* obj);
	~MeshRenderer() override = default;

	/// 指定されたパスのモデルをロードし、レンダリングの準備を行います。
	bool Load(const std::wstring& filePath, const std::wstring& pipelineName);

	/// コンポーネントの終了処理。
	void Term() override;

protected:
	/// モデルのワールド行列の更新を行います。
	void Update(float deltaTime) override;

	/// 描画パスへの登録や描画命令の実行を行います。
	void Draw(const RenderContext& context) override;

public:
	/// 現在ロードされているモデルのパスを取得します。
	[[nodiscard]] std::wstring GetModelPath() const { return m_ModelPath; }

	/// メッシュ用定数バッファのGPUハンドルを取得します。
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetMeshCBHandle() const { return m_MeshCB.GetHandleGPU(); }

	/// モデルがスキニングアニメーションを持っているか取得します。
	[[nodiscard]] bool HasAnimation() const { return m_HasAnimation; }
private:
	std::wstring m_ModelPath;					// ロードされたモデルのパス
	RenderSystem* m_RenderSystem = nullptr;	// システム全体で共有されるレンダーシステムへの参照
	ConstantBuffer m_MeshCB;					// このインスタンス固有のメッシュ定数バッファ

	bool m_HasAnimation = false;				// アニメーション対応モデルかどうか
};