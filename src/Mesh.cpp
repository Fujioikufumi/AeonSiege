//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Mesh.h"
#include "Logger.h"


///////////////////////////////////////////////////////////////////////////////
// Mesh class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
Mesh::Mesh()
: m_MaterialId(UINT32_MAX)
, m_IndexCount(0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理
//-----------------------------------------------------------------------------
bool Mesh::Init(ID3D12Device* pDevice, const ResMesh& resource)
{
    if (pDevice == nullptr)
    { return false; }

    if (!m_VertexBuffer.Init<MeshVertex>(
        pDevice, resource.Vertices.size(), resource.Vertices.data()))
    { return false; }

    if (!m_IndexBuffer.Init(
        pDevice, resource.Indices.size(), resource.Indices.data()))
    { return false; }

    m_MaterialId = resource.MaterialId;
    m_IndexCount = uint32_t(resource.Indices.size());
    m_IsSkinned = false;

    return true;
}

//-----------------------------------------------------------------------------
//	  初期化処理(スキニングメッシュ用)
//-----------------------------------------------------------------------------
bool Mesh::InitSkinned(ID3D12Device* pDevice, const ResMesh& resource)
{
    if (pDevice == nullptr)
    { return false; }

    if(resource.SkinnedVertices.empty())
    { return false; }

    if (!m_VertexBuffer.Init<SkinnedMeshVertex>(
        pDevice, resource.SkinnedVertices.size(), resource.SkinnedVertices.data()))
    { return false; }

    if (!m_IndexBuffer.Init(
        pDevice, resource.Indices.size(), resource.Indices.data()))
	{ return false; }

    m_MaterialId = resource.MaterialId;
    m_IndexCount = uint32_t(resource.Indices.size());
    m_IsSkinned = true;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void Mesh::Term()
{
    m_VertexBuffer.Term();
    m_IndexBuffer.Term();
    m_MaterialId = UINT32_MAX;
    m_IndexCount = 0;
}

//-----------------------------------------------------------------------------
//      描画処理
//-----------------------------------------------------------------------------
void Mesh::Draw(ID3D12GraphicsCommandList* pCmdList)
{
    if (m_IndexCount == 0)
    {
        ELOG("Error : Mesh has no indices to draw");
        return;
    }

    auto VBV = m_VertexBuffer.GetView();
    auto IBV = m_IndexBuffer.GetView();

    if (VBV.BufferLocation == 0 || IBV.BufferLocation == 0)
    {
        ELOG("Error : Mesh buffer views are invalid. VBV=0x%llx, IBV=0x%llx", VBV.BufferLocation, IBV.BufferLocation);
        return;
    }

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmdList->IASetVertexBuffers(0, 1, &VBV);
    pCmdList->IASetIndexBuffer(&IBV);
    pCmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}
//-----------------------------------------------------------------------------
//      マテリアルIDを取得
//-----------------------------------------------------------------------------
uint32_t Mesh::GetMaterialId() const
{ return m_MaterialId; }

//-----------------------------------------------------------------------------
//      頂点バッファビューを取得
//-----------------------------------------------------------------------------
D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexBufferView() const
{ return m_VertexBuffer.GetView();}

//-----------------------------------------------------------------------------
//      インデックスバッファビューを取得
//-----------------------------------------------------------------------------
D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView() const
{ return m_IndexBuffer.GetView(); }

//-----------------------------------------------------------------------------
//      インデックス数を取得
//-----------------------------------------------------------------------------
uint32_t Mesh::GetIndexCount() const
{ return m_IndexCount; }
