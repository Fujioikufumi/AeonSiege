//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "SkyDome.h"
#include "Logger.h"
#include "CommonStates.h"
#include "ShaderManager.h"
#include "GameManager.h"
#include "Scene.h"
#include "Camera.h"

#include <cmath>

namespace {

    struct SkyDomeVertex
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };

    //-----------------------------------------------------------------------------
    // CbSkyDome structure
    //-----------------------------------------------------------------------------
    struct alignas(256) CbSkyDome
    {
        XMFLOAT4X4 World;         
        XMFLOAT4X4 View;           
        XMFLOAT4X4 Proj;           
    };

} // namespace

///////////////////////////////////////////////////////////////////////////////
// SkyDome class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
SkyDome::SkyDome()
    : m_pPoolRes(nullptr)
    , m_Index(0)
    , m_IndexCount(0)
    , m_RatationY(0.0f)
{
}

//-----------------------------------------------------------------------------
//      デストラクタ
//-----------------------------------------------------------------------------
SkyDome::~SkyDome()
{
    Term();
}

//-----------------------------------------------------------------------------
//      初期化処理
//-----------------------------------------------------------------------------
bool SkyDome::Init
(
    ID3D12Device* pDevice,
    DescriptorPool* pPoolRes,
    DXGI_FORMAT         colorFormat,
    DXGI_FORMAT         depthFormat,
    float               radius,
    int                 slices,
    int                 stacks
)
{
    if (pDevice == nullptr || pPoolRes == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    m_pPoolRes = pPoolRes;
    m_pPoolRes->AddRef();

    if (!ShaderManager::GetInstance().LoadShader(L"SkyDome", L"SkyDomeVS.cso", L"SkyDomePS.cso"))
    {
        ELOG("Error : ShaderManager::LoadShader() Failed for SkyDome");
        return false;
    }

    ShaderInfo* shader = ShaderManager::GetInstance().GetShader(L"SkyDome");
    if (shader == nullptr || !shader->isValid)
    {
        ELOG("Error : SkyDome shader is invalid");
        return false;
    }

    // ���[�g�V�O�j�`���̐���
    {
        auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        D3D12_DESCRIPTOR_RANGE range[2] = {};

        range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        range[0].NumDescriptors = 1;
        range[0].BaseShaderRegister = 0;
        range[0].RegisterSpace = 0;
        range[0].OffsetInDescriptorsFromTableStart = 0;

        range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[1].NumDescriptors = 1;
        range[1].BaseShaderRegister = 0;
        range[1].RegisterSpace = 0;
        range[1].OffsetInDescriptorsFromTableStart = 0;

        // �X�^�e�B�b�N�T���v���[�̐ݒ�
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = -D3D12_FLOAT32_MAX;
        sampler.MaxLOD = +D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_PARAMETER param[2] = {};
        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[0].DescriptorTable.NumDescriptorRanges = 1;
        param[0].DescriptorTable.pDescriptorRanges = &range[0];
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges = &range[1];
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = 2;
        desc.NumStaticSamplers = 1;
        desc.pParameters = param;
        desc.pStaticSamplers = &sampler;
        desc.Flags = flag;

        ComPtr<ID3DBlob> pBlob;
        ComPtr<ID3DBlob> pErrorBlob;

        auto hr = D3D12SerializeRootSignature(
            &desc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            pBlob.GetAddressOf(),
            pErrorBlob.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3D12SerializeRootSignature() Failed. retcode = 0x%x", hr);
            return false;
        }

        hr = pDevice->CreateRootSignature(
            0,
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            IID_PPV_ARGS(m_pRootSig.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateRootSignature() Failed. retcode = 0x%x", hr);
            return false;
        }
    }

    {
        D3D12_INPUT_ELEMENT_DESC elements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_pRootSig.Get();
        desc.VS = { shader->pVSBlob->GetBufferPointer(), shader->pVSBlob->GetBufferSize() };
        desc.PS = { shader->pPSBlob->GetBufferPointer(), shader->pPSBlob->GetBufferSize() };
        desc.BlendState = DirectX::CommonStates::Opaque;
        desc.SampleMask = UINT_MAX;
        desc.RasterizerState = DirectX::CommonStates::CullNone;
        desc.DepthStencilState = DirectX::CommonStates::DepthRead;
        desc.InputLayout = { elements, 2 };
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = colorFormat;
        desc.DSVFormat = depthFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        auto hr = pDevice->CreateGraphicsPipelineState(
            &desc, IID_PPV_ARGS(m_pPSO.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
            return false;
        }
    }

    {
        std::vector<SkyDomeVertex> vertices;
        std::vector<uint32_t> indices;

		// 頂点データの生成
        for (int i = 0; i <= stacks; ++i)
        {
            float phi = phi = XM_PI * (static_cast<float>(i) / static_cast<float>(stacks) - 0.5f); // 0 ���� ��/2
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            for (int j = 0; j <= slices; ++j)
            {
                float theta = XM_2PI * static_cast<float>(j) / static_cast<float>(slices); // 0 ���� 2��
                float sinTheta = sinf(theta);
                float cosTheta = cosf(theta);

                SkyDomeVertex vertex;
                vertex.Position = XMFLOAT3(
                    radius * cosPhi * cosTheta,
                    radius * sinPhi,
                    radius * cosPhi * sinTheta
                );
                vertex.TexCoord = XMFLOAT2(
                    static_cast<float>(j) / static_cast<float>(slices),
                    1.0f - static_cast<float>(i) / static_cast<float>(stacks)
                );

                vertices.push_back(vertex);
            }
        }

        for (int i = 0; i < stacks; ++i)
        {
            for (int j = 0; j < slices; ++j)
            {
                int topLeft = i * (slices + 1) + j;
                int topRight = topLeft + 1;
                int bottomLeft = (i + 1) * (slices + 1) + j;
                int bottomRight = bottomLeft + 1;

                // �ŏ��̎O�p�`
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // 2�Ԗڂ̎O�p�`
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        m_IndexCount = static_cast<uint32_t>(indices.size());

        if (!m_VB.Init<SkyDomeVertex>(pDevice, vertices.size(), vertices.data()))
        {
            ELOG("Error : VertexBuffer::Init() Failed.");
            return false;
        }

        if (!m_IB.Init(pDevice, indices.size(), indices.data()))
        {
            ELOG("Error : IndexBuffer::Init() Failed.");
            return false;
        }
    }

    {
        for (auto i = 0; i < 2; ++i)
        {
            if (!m_CB[i].Init(pDevice, m_pPoolRes, sizeof(CbSkyDome)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }
        }
    }

    m_Index = 0;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void SkyDome::Term()
{
    for (auto i = 0; i < 2; ++i)
    {
        m_CB[i].Term();
    }

    m_IB.Term();
    m_VB.Term();

    m_pPSO.Reset();
    m_pRootSig.Reset();

    if (m_pPoolRes != nullptr)
    {
        m_pPoolRes->Release();
        m_pPoolRes = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      描画処理
//-----------------------------------------------------------------------------
void SkyDome::Draw
(
    ID3D12GraphicsCommandList* pCmd,
    D3D12_GPU_DESCRIPTOR_HANDLE handleTexture,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projMatrix
)
{
    if(m_RatationY > XM_2PI)
    {
        m_RatationY = 0.0f;
	}
    else
    {
        m_RatationY += 0.00015f;
    }

    Scene* scene = GameManager::GetScene();
    Camera* camera = scene->GetCamera();
	XMFLOAT3 cameraPos = camera->GetPosition();


    {
        auto ptr = m_CB[m_Index].GetPtr<CbSkyDome>();
        // カメラ位置（Viewの逆行列の平行移動）
        XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);

        // 回転は維持しつつ、位置は常にカメラへ追従
        XMMATRIX rotationMatrix = XMMatrixRotationY(m_RatationY);
		XMMATRIX translationMatrix = XMMatrixTranslation(cameraPos.x, cameraPos.y, cameraPos.z);

        XMMATRIX worldMatrix = XMMatrixMultiply(rotationMatrix, translationMatrix);

        XMStoreFloat4x4(&ptr->World, worldMatrix);

        XMStoreFloat4x4(&ptr->View, viewMatrix);
        XMStoreFloat4x4(&ptr->Proj, projMatrix);
        
    }

    auto vbv = m_VB.GetView();
    auto ibv = m_IB.GetView();

    pCmd->SetGraphicsRootSignature(m_pRootSig.Get());
    pCmd->SetGraphicsRootDescriptorTable(0, m_CB[m_Index].GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(1, handleTexture);
    pCmd->SetPipelineState(m_pPSO.Get());
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmd->IASetIndexBuffer(&ibv);
    pCmd->IASetVertexBuffers(0, 1, &vbv);
    pCmd->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);

    m_Index = (m_Index + 1) % 2;
}