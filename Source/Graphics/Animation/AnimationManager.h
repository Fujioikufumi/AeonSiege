#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ResMesh.h"
#include "SingletonManager.h"
#include <map>
#include <string>
#include <set>
#include <mutex>

//-----------------------------------------------------------------------------
//      アニメーションコントローラー
//-----------------------------------------------------------------------------
class AnimationManager : public SingletonManager<AnimationManager>
{
private:
	friend class SingletonManager<AnimationManager>;

	AnimationManager() = default;
	~AnimationManager() = default;
	AnimationManager(const AnimationManager&) = delete;
	void operator=(const AnimationManager&) = delete;

	// ボーン階層情報
	std::map<std::wstring, SkeletonInfo> m_Skeletons;

	// アニメーションクリップ情報
	std::map<std::wstring, std::vector<AnimationClip>> m_Animations;

	// 名前で管理するアニメーション
	std::map<std::string, AnimationClip> m_NamedAnimations;

	// アニメーションに含まれるボーン名
	std::map<std::string, std::set<std::string>> m_AnimationBoneNames;

	std::mutex m_Mutex;
public:
	bool Init();
	void Term();


	/// <summary>
	/// アニメーションファイルのロード(FBX)
	/// </summary>
	/// <param name="animationFilePath">アニメーションファイルのパス</param>
	/// <param name="animationName">アニメーションの名前</param>
	/// <returns>ロードに成功したかどうか</returns>
	bool LoadAnimation(
		const std::wstring& animationFilePath, 
		const std::string& animationName);

	/// <summary>
	/// 登録済みアニメーションを名前で取得
	/// </summary>
	/// <param name="animationName">アニメーションの名前</param>
	/// <returns>アニメーションクリップへのポインタ（存在しない場合は nullptr）</returns>
	const AnimationClip* GetAnimationByName(const std::string& animationName) const;

	/// <summary>
	/// アニメーションに含まれるボーン名一覧を取得
	/// </summary>
	/// <param name="animationName">アニメーションの名前</param>
	/// <returns>ボーン名のセットへのポインタ（存在しない場合は nullptr）</returns>
	const std::set<std::string>* GetAnimationBoneNames(const std::string& animationName) const;

	/// <summary>
	/// 指定モデルのスケルトン情報を登録
	/// </summary>
	/// <param name="modelPath">モデルのパス</param>
	/// <param name="skeleton">スケルトン情報</param>
	void RegisterSkeleton(const std::wstring& modelPath, const SkeletonInfo& skeleton);

	/// <summary>
	/// アニメーションクリップを登録
	/// </summary>
	/// <param name="modelPath">モデルのパス</param>
	/// <param name="animations">アニメーションクリップのリスト</param>
	void RegisterAnimations(const std::wstring& modelPath, const std::vector<AnimationClip>& animations);

	/// <summary>
	/// スケルトン情報を取得
	/// </summary>
	/// <param name="modelPath">モデルのパス</param>
	/// <returns>スケルトン情報へのポインタ（存在しない場合は nullptr）</returns>
	const SkeletonInfo* GetSkeleton(const std::wstring& modelPath) const;

	/// <summary>
	/// アニメーションクリップ一覧を取得
	/// </summary>
	/// <param name="modelPath">モデルのパス</param>
	/// <returns>アニメーションクリップのリストへのポインタ（存在しない場合は nullptr）</returns>
	const std::vector<AnimationClip>* GetAnimations(const std::wstring& modelPath) const;

	/// <summary>
	/// アニメーションクリップを名前で取得
	/// </summary>
	/// <param name="modelPath">モデルのパス</param>
	/// <param name="clipName">クリップの名前</param>
	/// <returns>アニメーションクリップへのポインタ（存在しない場合は nullptr）</returns>
	const AnimationClip* GetAnimationClip(const std::wstring& modelPath, const std::string& clipName) const;

	/// <summary>
	/// 登録済みアニメーション名の一覧を取得
	/// </summary>
	/// <returns>登録済みアニメーション名のリスト</returns>
	std::vector<std::string> GetAvailableAnimationNames() const;

	/// <summary>
	/// 指定モデルパスに紐づくスケルトン・パス別クリップ一覧を削除する。
	/// LoadModel 時に RegisterSkeleton / RegisterAnimations で登録した分のみ対象。
	/// （LoadAnimation で登録した名前付きクリップ m_NamedAnimations は別管理のため消さない）
	/// </summary>
	void UnregisterModel(const std::wstring& modelPath);
};