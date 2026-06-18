#pragma once
#include <string>
#include <vector>
#include <d3d12.h>

#include "Graphics/Model/ResMesh.h"     // ResMaterial
#include "Graphics/Renderer/Material.h"    // Material
#include "Graphics/D3D12/DescriptorPool.h"

class MaterialResolver
{
public:
    /// <summary>
	/// モデルのマテリアル情報をもとに、Materialクラスのインスタンスを生成します.
    /// </summary>
    static bool Build(
        ID3D12Device* device,
        DescriptorPool* pool,
        ID3D12CommandQueue* queue,
        const std::wstring& modelDirectory,
        const std::wstring& modelPath,
        const std::vector<ResMaterial>& resMaterial,
        Material& outMaterial);

private:
    static std::wstring GetFileNameOnly(const std::wstring& p);
    static std::wstring EnsureDirSlash(std::wstring dir);
    static std::wstring GetModelBaseName(const std::wstring& modelPath);
    static bool FileExists(const std::wstring& path);

    // filename-only + (modelDir) + (modelDir\<Model>.fbm\)
    static std::wstring ResolveTexturePath_FileNameOnly(
        const std::wstring& fileNameOrPath,
        const std::wstring& modelDirectory,
        const std::wstring& modelPath);
};