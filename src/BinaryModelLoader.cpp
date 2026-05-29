#include "BinaryModelLoader.h"
#include <fstream>
#include <iostream>

bool BinaryModelLoader::SupportsExtension(const std::wstring& extension) const
{
    // 大文字小文字の揺れを考慮する場合は要変換
    return (extension == L".bmdl" || extension == L".BMDL");
}

bool BinaryModelLoader::Load(
    const wchar_t* filename,
    std::vector<ResMesh>& meshes,
    std::vector<ResMaterial>& materials,
    SkeletonInfo* pSkeleton,
    std::vector<AnimationClip>* pAnimations)
{
    // バイナリモードでファイルを開く
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open())
    {
        return false;
    }

    // 1. ヘッダの読み込み
    ModelFileHeader header = {};
    inFile.read(reinterpret_cast<char*>(&header), sizeof(ModelFileHeader));

    // マジックナンバーが正しいかチェック
    if (header.magic != MODEL_MAGIC)
    {
        return false; // 不正なファイルフォーマット
    }

    // 2. マテリアル情報の読み込み
    materials.resize(header.numMaterials);
    for (uint32_t i = 0; i < header.numMaterials; ++i)
    {
        MaterialData matData;
        inFile.read(reinterpret_cast<char*>(&matData), sizeof(MaterialData));

        ResMaterial resMat = {};
        resMat.Diffuse = matData.diffuse;
        // char配列からstd::wstringへの変換（UTF-8等環境に応じて適宜調整が必要な場合があります）
        resMat.DiffuseMap = std::wstring(matData.diffuseMapName, matData.diffuseMapName + strlen(matData.diffuseMapName));
        resMat.NormalMap = std::wstring(matData.normalMapName, matData.normalMapName + strlen(matData.normalMapName));

        materials[i] = resMat;
    }

    // 3. ボーン（スケルトン）情報の読み込み
    if (pSkeleton && header.numBones > 0)
    {
        pSkeleton->bones.resize(header.numBones);
        for (uint32_t i = 0; i < header.numBones; ++i)
        {
            BoneData boneData;
            inFile.read(reinterpret_cast<char*>(&boneData), sizeof(BoneData));

            BoneInfo info = {};
            info.name = boneData.name;
            info.parentIndex = boneData.parentIndex;
            info.hasBind = boneData.hasBind;
            info.localTransform = DirectX::XMLoadFloat4x4(&boneData.localTransform);
            info.boneBindInv = DirectX::XMLoadFloat4x4(&boneData.boneBindInv);
            info.boneBindGlobal = DirectX::XMLoadFloat4x4(&boneData.boneBindGlobal);

            pSkeleton->bones[i] = info;
            pSkeleton->boneNameToIndex[info.name] = i; // 検索用マップの構築
        }
    }
    else if (header.numBones > 0)
    {
        // 呼び出し元がスケルトンを要求していない場合は、ファイルポインタを進めてスキップする
        inFile.seekg(header.numBones * sizeof(BoneData), std::ios::cur);
    }

    // 4. メッシュ（頂点とインデックス）情報の読み込み
    meshes.resize(header.numMeshes);
    for (uint32_t i = 0; i < header.numMeshes; ++i)
    {
        MeshHeader meshHeader;
        inFile.read(reinterpret_cast<char*>(&meshHeader), sizeof(MeshHeader));

        ResMesh resMesh = {};
        resMesh.MaterialId = meshHeader.materialId;
        resMesh.HasAnimation = meshHeader.isSkinned;

        // 頂点データの読み込み（vectorのresizeでメモリ領域を確保してから、そこに直接一括コピー！）
        if (meshHeader.isSkinned)
        {
            resMesh.SkinnedVertices.resize(meshHeader.numVertices);
            if (meshHeader.numVertices > 0)
            {
                inFile.read(reinterpret_cast<char*>(resMesh.SkinnedVertices.data()),
                    sizeof(SkinnedMeshVertex) * meshHeader.numVertices);
            }
        }
        else
        {
            resMesh.Vertices.resize(meshHeader.numVertices);
            if (meshHeader.numVertices > 0)
            {
                inFile.read(reinterpret_cast<char*>(resMesh.Vertices.data()),
                    sizeof(MeshVertex) * meshHeader.numVertices);
            }
        }

        // インデックスデータの読み込み（同様に一括コピー）
        resMesh.Indices.resize(meshHeader.numIndices);
        if (meshHeader.numIndices > 0)
        {
            inFile.read(reinterpret_cast<char*>(resMesh.Indices.data()),
                sizeof(uint32_t) * meshHeader.numIndices);
        }

        meshes[i] = resMesh;
    }

    inFile.close();
    return true;
}