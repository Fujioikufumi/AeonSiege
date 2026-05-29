#pragma once
#include <d3d12.h>
#include <string>

class DescriptorPool;

/// <summary>
/// コンピュートパイプラインをコマンドリストにバインドする
/// </summary>
/// <param name="pCmdList"></param>
/// <param name="pipelineName"></param>
/// <param name="pPool"></param>
/// <returns></returns>
bool BindComputePipeline(
    ID3D12GraphicsCommandList* pCmdList,
    const std::wstring& pipelineName,
    DescriptorPool* pPool);
