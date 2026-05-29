#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <ResMesh.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>


//-----------------------------------------------------------------------------
//      メッシュクラス
//-----------------------------------------------------------------------------
class Mesh
{
public:
	Mesh();
	virtual ~Mesh();

	//　初期化処理
	bool Init(ID3D12Device* pDevice, const ResMesh& resource);

	//　初期化処理(スキニングメッシュ用)
	bool InitSkinned(ID3D12Device* pDevice, const ResMesh& resource);

	//　終了処理
	void Term();

	// 描画処理
	void Draw(ID3D12GraphicsCommandList* pCmdList);

	// マテリアルIDの取得
	[[nodiscard]] uint32_t GetMaterialId() const;

	// 頂点バッファビューの取得
	[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

	// インデックスバッファビューの取得
	[[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

	// インデックス数の取得
	[[nodiscard]] uint32_t GetIndexCount() const;

	// スキニングメッシュ判定の取得
	[[nodiscard]] bool IsSkinned() const { return m_IsSkinned; }

private:
	VertexBuffer    m_VertexBuffer;     // 頂点バッファ
	IndexBuffer     m_IndexBuffer;      // インデックスバッファ
	uint32_t        m_MaterialId;       // マテリアルID
	uint32_t        m_IndexCount;       // インデックス数
	bool 		    m_IsSkinned;        // スキニングメッシュかどうか

	Mesh(const Mesh&) = delete;     // コピー禁止
	void operator = (const Mesh&) = delete;
};
