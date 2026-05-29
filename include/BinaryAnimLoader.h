#pragma once
#pragma once
#include "IModelLoader.h"
#include "ModelFormat.h"

class BinaryAnimLoader : public IModelLoader
{
public:
    BinaryAnimLoader() = default;
    ~BinaryAnimLoader() override = default;

    /// <summary>
	/// アニメーションファイル（.banm）を読み込む
    /// </summary>
    /// <param name="filename">読み込むアニメーションファイルのパス</param>
    /// <param name="meshes">読み込んだメッシュデータを格納するベクター</param>
    /// <param name="materials">読み込んだマテリアルデータを格納するベクター</param>
    /// <param name="pSkeleton">読み込んだスケルトン情報を格納するポインタ（オプション）</param>
    /// <param name="pAnimations">読み込んだアニメーションクリップを格納するポインタ（オプション）</param>
    /// <returns>読み込みに成功した場合はtrue、失敗した場合はfalse</returns>
    bool Load(
        const wchar_t* filename,
        std::vector<ResMesh>& meshes,
        std::vector<ResMaterial>& materials,
        SkeletonInfo* pSkeleton = nullptr,
        std::vector<AnimationClip>* pAnimations = nullptr) override;

    bool SupportsExtension(const std::wstring& extension) const override;
    const char* GetLoaderName() const override { return "BinaryAnimLoader"; }
};