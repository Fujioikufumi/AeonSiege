#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <string>
#include <vector>
#include <map>
#include "main.h"


//-----------------------------------------------------------------------------
//      ボーン情報
//-----------------------------------------------------------------------------
struct BoneInfo
{
	std::string name;                   // ボーン名
	int parentIndex;                    // 親ボーンのインデックス
	XMMATRIX localTransform;            // ローカル変換行列

	XMMATRIX boneBindInv = XMMatrixIdentity();
	bool hasBind = false;

	XMMATRIX boneBindGlobal = XMMatrixIdentity();

	int bindParentIndex = -1;

	// 追加：bindParent基準のローカル（bind pose）
	XMMATRIX bindLocal = XMMatrixIdentity();
};


//-----------------------------------------------------------------------------
// 	 行列キーフレーム
//   時間ごとの変換行列のリスト
//-----------------------------------------------------------------------------
struct MatrixKeyFrame
{
	float time;        // 時間
	XMFLOAT4X4 matrix; // 変換行列
};

//-----------------------------------------------------------------------------
//   アニメーションチャンネル
//   一つのボーンのアニメーション情報
//-----------------------------------------------------------------------------
struct AnimationChannel
{
	std::string boneName;               // ボーン名
	std::vector<MatrixKeyFrame> matrixKeys; // 行列キーフレーム
};

//-----------------------------------------------------------------------------
//   アニメーションクリップ
//   一つのアニメーション全体の情報
//-----------------------------------------------------------------------------
struct AnimationClip
{
	std::string name;                               // アニメーション名
	float duration = 0.0f;                          // アニメーションの長さ
	float ticksPerSecond = 0.0f;                    // ティック/秒
	std::vector<AnimationChannel> channels;     // アニメーションチャンネル
};

//-----------------------------------------------------------------------------
//  スキニング情報付き頂点
//-----------------------------------------------------------------------------
class SkinnedMeshVertex
{
public:
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT4 Tangent;
	XMUINT4 BoneIndices;  // 影響するボーンのインデックス
	XMFLOAT4 BoneWeights; // ボーンの重み

	SkinnedMeshVertex() = default;

	SkinnedMeshVertex(
		XMFLOAT3 const& position,
		XMFLOAT3 const& normal,
		XMFLOAT2 const& texcoord,
		XMFLOAT4 const& tangent,
		XMUINT4 const& boneIndices,
		XMFLOAT4 const& boneWeights
	)
		: Position(position)
		, Normal(normal)
		, TexCoord(texcoord)
		, Tangent(tangent)
		, BoneIndices(boneIndices)
		, BoneWeights(boneWeights)
	{
	}

	// 入力レイアウト
	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	// 入力要素
	static const int InputElementsCount = 6;
	// 入力要素配列
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementsCount];

};

//-----------------------------------------------------------------------------
// MeshVertex structure
//-----------------------------------------------------------------------------
class MeshVertex
{
public:
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Tangent;

	MeshVertex() = default;

	MeshVertex(
		XMFLOAT3 const& position,
		XMFLOAT3 const& normal,
		XMFLOAT2 const& texcoord,
		XMFLOAT3 const& tangent
	)
		: Position(position)
		, Normal(normal)
		, TexCoord(texcoord)
		, Tangent(tangent)
	{
	}

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const int InputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

//-----------------------------------------------------------------------------
//      スケルトン情報
//-----------------------------------------------------------------------------
struct SkeletonInfo
{
	std::vector<BoneInfo> bones;                // ボーンの情報
	int rootBoneIndex = -1;                     // ルートボーンインデックス
	std::map<std::string, int> boneNameToIndex; // ボーン名からインデックスへのマップ

	XMMATRIX meshBind = XMMatrixIdentity();
	XMMATRIX meshBindInv = XMMatrixIdentity();
};

//-----------------------------------------------------------------------------
//      メッシュデータ構造体
//-----------------------------------------------------------------------------
struct ResMesh
{
	std::vector<MeshVertex> Vertices;               // 通常の頂点
	std::vector<SkinnedMeshVertex> SkinnedVertices; // スキニング頂点
	std::vector<uint32_t> Indices;
	uint32_t MaterialId;

	// 後で削除
	bool HasAnimation = false;                      // アニメーションがあるかどうか
};

//-----------------------------------------------------------------------------
// 	    マテリアルデータ構造体
//-----------------------------------------------------------------------------
struct ResMaterial
{
	XMFLOAT3   Diffuse;        // 拡散反射成分です.
	XMFLOAT3   Specular;       // 鏡面反射成分です.
	float               Alpha;          // 透過成分です.
	float               Shininess;      // 鏡面反射強度です.
	std::wstring        DiffuseMap;     // ディフューズマップファイルパスです.
	std::wstring        SpecularMap;    // スペキュラーマップファイルパスです.
	std::wstring        ShininessMap;   // シャイネスマップファイルパスです.
	std::wstring        NormalMap;      // 法線マップファイルパスです.
};