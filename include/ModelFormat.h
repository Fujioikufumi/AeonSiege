#pragma once
#pragma once
#include <stdint.h>
#include <DirectXMath.h>

// マジックナンバー（ファイルが正しい形式か判定するための識別子）
const uint32_t MODEL_MAGIC = 'MDL1'; // Model version 1
const uint32_t ANIM_MAGIC = 'ANM1'; // Animation version 1

//=============================================================================
// 1. モデルファイル (.bmdl) 用のヘッダと構造体
//=============================================================================

// モデルファイル全体のヘッダ情報（ファイル先頭に必ず置く）
struct ModelFileHeader
{
    uint32_t magic;          // 'MDL1'
    uint32_t numMeshes;      // サブメッシュの数
    uint32_t numMaterials;   // マテリアルの数
    uint32_t numBones;       // ボーン（スケルトン）の数
    bool     hasAnimation;   // スキニングメッシュかどうか
    char     reserved[3];    // パディング（4バイト境界に合わせるため）
};

// マテリアル情報の保存用（ファイルパスは固定長か、サイズ＋文字列で保存する）
struct MaterialData
{
    DirectX::XMFLOAT3 diffuse;
    char diffuseMapName[256]; // テクスチャのファイル名（簡易的に固定長配列にするのが最も読み書きが楽）
    char normalMapName[256];
};

// 各サブメッシュ（ResMesh）のヘッダ情報
struct MeshHeader
{
    uint32_t materialId;
    uint32_t numVertices;
    uint32_t numIndices;
    bool     isSkinned;      // 頂点が SkinnedMeshVertex か MeshVertex か
    char     reserved[3];
};

// ボーン（SkeletonInfo::BoneInfo）の保存用
struct BoneData
{
    char name[64];              // ボーン名
    int  parentIndex;           // 親インデックス
    DirectX::XMFLOAT4X4 localTransform;
    DirectX::XMFLOAT4X4 boneBindInv;
    DirectX::XMFLOAT4X4 boneBindGlobal;
    bool hasBind;
    char reserved[3];
};


//=============================================================================
// 2. アニメーションファイル (.banm) 用のヘッダと構造体
//=============================================================================

// アニメーション全体のヘッダ情報
struct AnimFileHeader
{
    uint32_t magic;          // 'ANM1'
    uint32_t numChannels;    // 動くボーン（チャンネル）の数
    float    duration;       // アニメーションの長さ
    float    ticksPerSecond; // フレームレート
    char     clipName[64];   // アニメーション名（例: "Run"）
};

// 各チャンネル（ボーン単位のアニメーション）のヘッダ
struct AnimChannelHeader
{
    char     boneName[64];   // どのボーンか
    uint32_t numKeyFrames;   // キーフレーム数
};

// MatrixKeyFrame の保存用（ResMesh.h の MatrixKeyFrame と同じ構造）
struct KeyFrameData
{
    float time;
    DirectX::XMFLOAT4X4 matrix;
};