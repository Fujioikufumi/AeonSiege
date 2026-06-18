#pragma once
#include "Component.h"
#include "ResMesh.h"
#include "AnimationManager.h"
#include "ConstantBuffer.h"
#include <string>
#include <unordered_map>
#include <DirectXMath.h>

/// <summary>
/// アニメーションの制御を行うコンポーネント。
/// ボーン行列の計算と定数バッファへの書き込みを担当します。
/// </summary>
class AnimationController : public Component
{
public:
	// ボーンの最大数。
	static constexpr size_t kMaxBones = 128;

	// ボーン行列をGPUに送るための定数バッファ構造体
	struct alignas(16) BoneMatrixBuffer
	{
		DirectX::XMFLOAT4X4 matrices[kMaxBones];
	};

public:
	explicit AnimationController(GameObject* obj);
	~AnimationController() override;
	bool Init() override;
	bool Init(const std::wstring& modelPath);
	void Term() override;

	// アニメーションを再生
	bool Play(const std::string& animationName);

	// アニメーションを停止
	void Stop();

	// アニメーションを一時停止
	void Pause();

	// 一時停止したアニメーションを再開
	void Resume();

	// アニメーションクリップを直接設定
	void SetAnimationClip(const std::wstring& modelPath, const std::string& clipName);

	// アニメーションの再生速度を設定
	void SetSpeed(float speed) { m_Speed = speed; }

	// アニメーションの現在時間を設定
	void SetTime(float time) { m_CurrentTime = time; }

	// ループ再生の有効/無効を設定
	void SetLoop(bool loop) { m_IsLoop = loop; }

	// ボーンマトリックス定数バッファのGPUハンドルを取得
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetBoneMatrixCBHandle() const { return m_BoneMatrixCB.GetHandleGPU(); }

	// ボーン数を取得
	[[nodiscard]] size_t GetBoneCount() const { return m_BoneMatrices.size(); }

	// 初期化済みかどうかを取得
	[[nodiscard]] bool IsInitialized() const { return m_IsInitialized; }

	// アニメーション再生中かどうかを取得
	[[nodiscard]] bool IsPlaying() const { return m_IsPlaying; }

	// アニメーションが明示的に一時停止されているか取得
	[[nodiscard]] bool IsPaused() const { return m_HoldPaused; }

	// 現在のアニメーション名を取得
	[[nodiscard]] std::string GetCurrentAnimationName() const { return m_CurrentAnimationName; }

protected:
	// 更新処理。アニメーション時間の進捗と行列計算を行います。
	void Update(float deltaTime) override;

private:
	// ボーン行列を現在の時間に基づいて計算します。
	void CalculateBoneMatrices();

	// ボーン名からアニメーションチャンネルを高速に検索するためのルックアップを再構築します。
	void RebuildChannelLookup();

	// 行列計算の結果をGPUの定数バッファに書き込みます。
	void UpdateConstantBuffer();

private:
	bool m_IsInitialized = false;			// 初期化フラグ
	bool m_IsPlaying = false;				// 再生中フラグ
	bool m_IsLoop = true;					// ループ再生フラグ
	bool m_HoldPaused = false;				// 一時停止フラグ

	// --- 再生パラメータ ---
	float m_CurrentTime = 0.0f;				// 現在のアニメーション時間
	float m_Speed = 1.0f;					// 再生速度

	// --- リソース参照 ---
	std::wstring m_ModelPath;						// 使用しているモデルのパス
	std::string m_CurrentAnimationName;				// 現在再生中のアニメーション名
	const AnimationClip* m_pCurrentClip = nullptr;	// 現在のアニメーションクリップ
	const SkeletonInfo* m_pSkeleton = nullptr;		// スケルトン情報（ボーン構造）

	// --- 計算データ・バッファ ---
	std::vector<DirectX::XMMATRIX> m_BoneMatrices;	// 計算されたボーン行列
	ConstantBuffer m_BoneMatrixCB;					// ボーン行列用定数バッファ

	// ボーン名からアニメーションチャンネルへの検索を高速化するためのマップ
	std::unordered_map<std::string, const AnimationChannel*> m_ChannelByBoneName;
};