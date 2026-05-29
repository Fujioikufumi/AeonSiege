#pragma once
//---------------------------------------------------------------
// Includes
//---------------------------------------------------------------
#include "ResMesh.h"
#include <string>

//---------------------------------------------------------------
//		モデルローダーインターフェース
//---------------------------------------------------------------
class IModelLoader
{
public:
	virtual ~IModelLoader() = default;

	/// <summary>
	/// モデルのロード
	/// </summary>
	/// <param name="filename">モデルファイルのパス</param>
	/// <param name="meshes">読み込んだメッシュ情報を格納するベクター</param>
	/// <param name="materials">読み込んだマテリアル情報を格納するベクター</param>
	/// <param name="pSkeleton">スケルトン情報を格納するポインタ（FBXの場合のみ使用）</param>
	/// <param name="pAnimations">アニメーションクリップ情報を格納するポインタ（FBXの場合のみ使用）</param>
	/// <returns>ロード成功ならtrue、失敗ならfalse</returns>
	virtual bool Load(
		const wchar_t* filename,
		std::vector<ResMesh>& meshes,
		std::vector<ResMaterial>& materials,
		SkeletonInfo* pSkeleton = nullptr,
		std::vector<AnimationClip>* pAnimations = nullptr) = 0;
	
	/// <summary>
	/// このローダーが指定されたファイル拡張子をサポートしているかどうかを確認
	/// </summary>
	/// <param name="extension">ファイル拡張子（例: ".fbx"）</param>
	/// <returns>サポートしていればtrue、サポートしていなければfalse</returns>
	virtual bool SupportsExtension(const std::wstring& extension) const = 0;

	/// <summary>
	/// ローダーの名前を取得（デバッグ用）
	/// </summary>
	/// <returns>ローダーの名前</returns>
	virtual const char* GetLoaderName() const = 0;
};