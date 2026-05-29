#pragma once
#include "IModelLoader.h"
#include "ModelFormat.h"

class BinaryModelLoader : public IModelLoader
{
public:
    BinaryModelLoader() = default;
    ~BinaryModelLoader() override = default;
    bool Load(
        const wchar_t* filename,
        std::vector<ResMesh>& meshes,
        std::vector<ResMaterial>& materials,
        SkeletonInfo* pSkeleton = nullptr,
        std::vector<AnimationClip>* pAnimations = nullptr) override;
    bool SupportsExtension(const std::wstring& extension) const override;
    const char* GetLoaderName() const override { return "BinaryModelLoader"; }
};