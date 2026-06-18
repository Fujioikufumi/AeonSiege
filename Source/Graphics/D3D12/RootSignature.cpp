#include "RootSignature.h"
#include "Logger.h"
#include <cstring>

//-----------------------------------------------------------------------------
//      Desc
//-----------------------------------------------------------------------------
RootSignature::Desc::Desc()
	: m_Flags(0)
{
	std::memset(&m_RootDesc, 0, sizeof(m_RootDesc));
	for (int i = 0; i < 5; ++i) { m_DenyStage[i] = true; }
}

RootSignature::Desc::~Desc()
{
	m_Ranges.clear();
	m_Samplers.clear();
	m_Params.clear();
}

RootSignature::Desc& RootSignature::Desc::Begin(int count)
{
	m_Flags = 0;
	for (int i = 0; i < 5; ++i) { m_DenyStage[i] = true; }
	std::memset(&m_RootDesc, 0, sizeof(m_RootDesc));

	m_Samplers.clear();
	m_Ranges.resize(count);
	m_Params.resize(count);
	return *this;
}

void RootSignature::Desc::CheckStage(ShaderStage stage)
{
	int index = static_cast<int>(stage) - 1;
	if (0 <= index && index < 5) { m_DenyStage[index] = false; }
}

void RootSignature::Desc::SetParam(ShaderStage stage, int index, uint32_t shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE type)
{
	if (index >= static_cast<int>(m_Params.size())) { return; }

	m_Ranges[index].RangeType = type;
	m_Ranges[index].NumDescriptors = 1;
	m_Ranges[index].BaseShaderRegister = shaderRegister;
	m_Ranges[index].RegisterSpace = 0;
	m_Ranges[index].OffsetInDescriptorsFromTableStart = 0;

	m_Params[index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_Params[index].DescriptorTable.NumDescriptorRanges = 1;
	m_Params[index].DescriptorTable.pDescriptorRanges = &m_Ranges[index];
	m_Params[index].ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(stage);
	CheckStage(stage);
}

RootSignature::Desc& RootSignature::Desc::SetCBV(ShaderStage stage, int index, uint32_t shaderRegister)
{
	SetParam(stage, index, shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	return *this;
}

RootSignature::Desc& RootSignature::Desc::SetSRV(ShaderStage stage, int index, uint32_t shaderRegister)
{
	SetParam(stage, index, shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	return *this;
}

RootSignature::Desc& RootSignature::Desc::SetUAV(ShaderStage stage, int index, uint32_t shaderRegister)
{
	SetParam(stage, index, shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	return *this;
}

RootSignature::Desc& RootSignature::Desc::SetSmp(ShaderStage stage, int index, uint32_t shaderRegister)
{
	SetParam(stage, index, shaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
	return *this;
}

RootSignature::Desc& RootSignature::Desc::AddStaticSmp(ShaderStage stage, uint32_t shaderRegister, SamplerState state)
{
	D3D12_STATIC_SAMPLER_DESC staticDesc = {};

	staticDesc.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
	staticDesc.MaxAnisotropy = 1;
	staticDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticDesc.MinLOD = 0.0f;
	staticDesc.MaxLOD = D3D12_FLOAT32_MAX;
	staticDesc.ShaderRegister = shaderRegister;
	staticDesc.RegisterSpace = 0;
	staticDesc.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(stage);
	CheckStage(stage);

	switch (state)
	{
	case SamplerState::PointWrap:
		staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		break;

	case SamplerState::PointClamp:
		staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		break;

	case SamplerState::LinearWrap:
		staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		break;

	case SamplerState::LinearClamp:
		staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		break;

	case SamplerState::AnisotropicWrap:
		staticDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticDesc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		break;

	case SamplerState::AnisotropicClamp:
		staticDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticDesc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		break;

	case SamplerState::ShadowComparison:
		staticDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		staticDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		break;
	}

	m_Samplers.push_back(staticDesc);
	return *this;
}

RootSignature::Desc& RootSignature::Desc::AllowIL()
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	return *this;
}

RootSignature::Desc& RootSignature::Desc::AllowSO()
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
	return *this;
}

RootSignature::Desc& RootSignature::Desc::AddShadowSampler(ShaderStage stage, uint32_t shaderRegister)
{
	return AddStaticSmp(stage, shaderRegister, SamplerState::ShadowComparison);
}

RootSignature::Desc& RootSignature::Desc::End()
{
	if (m_DenyStage[0]) m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	if (m_DenyStage[1]) m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	if (m_DenyStage[2]) m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	if (m_DenyStage[3]) m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	if (m_DenyStage[4]) m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	m_RootDesc.NumParameters = static_cast<UINT>(m_Params.size());
	m_RootDesc.pParameters = m_Params.empty() ? nullptr : m_Params.data();
	m_RootDesc.NumStaticSamplers = static_cast<UINT>(m_Samplers.size());
	m_RootDesc.pStaticSamplers = m_Samplers.empty() ? nullptr : m_Samplers.data();
	m_RootDesc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(m_Flags);

	return *this;
}

const D3D12_ROOT_SIGNATURE_DESC* RootSignature::Desc::GetDesc() const
{
	return &m_RootDesc;
}

//-----------------------------------------------------------------------------
//      RootSignature
//-----------------------------------------------------------------------------
RootSignature::RootSignature() : m_RootSignature(nullptr) {}
RootSignature::~RootSignature() { Term(); }

bool RootSignature::Init(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC* desc)
{
	if (!device || !desc) return false;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT result = D3D12SerializeRootSignature(desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(result)) { return false; }

	result = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
	return SUCCEEDED(result);
}

void RootSignature::Term() { m_RootSignature.Reset(); }

ID3D12RootSignature* RootSignature::GetPtr() const { return m_RootSignature.Get(); }