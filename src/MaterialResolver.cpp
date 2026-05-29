#include "MaterialResolver.h"
#include <Windows.h>
#include <algorithm>
#include "Logger.h"
#include "NameSpace.h"
#include <ResourceUploadBatch.h> // DirectXTK

std::wstring MaterialResolver::GetFileNameOnly(const std::wstring& p)
{
    if (p.empty()) return std::wstring();
    const size_t pos = p.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? p : p.substr(pos + 1);
}

std::wstring MaterialResolver::EnsureDirSlash(std::wstring dir)
{
    for (auto& ch : dir)
    {
        if (ch == L'/') ch = L'\\';
    }
    if (!dir.empty() && dir.back() != L'\\') dir.push_back(L'\\');
    return dir;
}

bool MaterialResolver::FileExists(const std::wstring& path)
{
    if (path.empty()) return false;
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

std::wstring MaterialResolver::GetModelBaseName(const std::wstring& modelPath)
{
    size_t lastSlash = modelPath.find_last_of(L"\\/");
    size_t lastDot = modelPath.find_last_of(L'.');

    if (lastDot == std::wstring::npos) lastDot = modelPath.size();
    if (lastSlash == std::wstring::npos) lastSlash = (size_t)-1;

    if (lastDot > lastSlash + 1)
    {
        return modelPath.substr(lastSlash + 1, lastDot - (lastSlash + 1));
    }
    return std::wstring();
}

std::wstring MaterialResolver::ResolveTexturePath_FileNameOnly(
    const std::wstring& fileNameOrPath,
    const std::wstring& modelDirectory,
    const std::wstring& modelPath)
{
    const std::wstring fileOnly = GetFileNameOnly(fileNameOrPath);
    if (fileOnly.empty()) return std::wstring();

    const std::wstring dir = EnsureDirSlash(modelDirectory);

    // 1) modelDirectory
    {
        std::wstring cand = dir + fileOnly;
        if (FileExists(cand)) return cand;
    }

    // 2) modelDirectory\<Model>.fbm\
    {
    const std::wstring base = GetModelBaseName(modelPath);
    if (!base.empty())
    {
        std::wstring cand = dir + base + L".fbm\\" + fileOnly;
        if (FileExists(cand)) return cand;
    }

return std::wstring();
}

bool MaterialResolver::Build(
    ID3D12Device* device,
    DescriptorPool* pool,
    ID3D12CommandQueue* queue,
    const std::wstring& modelDirectory,
    const std::wstring& modelPath,
    const std::vector<ResMaterial>& resMaterial,
    Material& outMaterial)
{
    if (!device || !pool || !queue)
    {
        ELOG("MaterialResolver::Build: invalid device/pool/queue");
        return false;
    }

    const size_t count = std::max<size_t>(1, resMaterial.size());

    // 1) Material(CB) ������
    if (!outMaterial.Init(device, pool, sizeof(CbMaterial), count))
    {
        ELOG("MaterialResolver::Build: Material::Init failed");
        return false;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const ResMaterial rm = (i < resMaterial.size()) ? resMaterial[i] : ResMaterial{};

        auto* cb = outMaterial.GetBufferPtr<CbMaterial>(i);
        if (!cb) continue;

        cb->BaseColor = XMFLOAT3(rm.Diffuse.x, rm.Diffuse.y, rm.Diffuse.z);
        cb->Alpha = rm.Alpha;
        cb->Roughness = 1.0f - (rm.Shininess / 100.0f);
        cb->Metallic = 0.0f;

        cb->HasBaseColorMap = 0;
        cb->HasMetallicMap = 0;
        cb->HasRoughnessMap = 0;
        cb->HasNormalMap = 0;
    }

    DirectX::ResourceUploadBatch batch(device);
    batch.Begin();

    for (size_t i = 0; i < count; ++i)
    {
        const ResMaterial rm = (i < resMaterial.size()) ? resMaterial[i] : ResMaterial{};

        auto* cb = outMaterial.GetBufferPtr<CbMaterial>(i);

        // BaseColor (t0)
        {
            std::wstring resolvedBase = ResolveTexturePath_FileNameOnly(rm.DiffuseMap, modelDirectory, modelPath);
            if (!resolvedBase.empty())
            {
                if (cb) cb->HasBaseColorMap = 1;
                if (!outMaterial.SetTexture(i, TU_BASE_COLOR, resolvedBase, batch))
                {
                    ELOG("MaterialResolver: SetTexture(TU_BASE_COLOR) failed: %ls", resolvedBase.c_str());
                }
            }

            std::wstring resolvedNormal = ResolveTexturePath_FileNameOnly(rm.NormalMap, modelDirectory, modelPath);
            if (!resolvedNormal.empty())
            {
                if (cb) cb->HasNormalMap = 1;
                if (!outMaterial.SetTexture(i, TU_NORMAL, resolvedNormal, batch))
                {
                    ELOG("MaterialResolver: SetTexture(TU_NORMAL) failed: %ls", resolvedNormal.c_str());
                }
            }

            DLOG("MaterialResolver: matId=%zu DiffuseMap='%ls' -> '%ls'",
                i, rm.DiffuseMap.c_str(), resolvedBase.c_str());
            DLOG("MaterialResolver: matId=%zu NormalMap='%ls' -> '%ls'",
                i, rm.NormalMap.c_str(), resolvedNormal.c_str());
        }


    }

    auto future = batch.End(queue);
    future.wait();

    DLOG("MaterialResolver::Build: count=%zu dir='%ls' model='%ls'",
        count, modelDirectory.c_str(), modelPath.c_str());

    return true;
}

