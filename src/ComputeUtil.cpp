#include "ComputeUtil.h"
#include "PipelineStateManager.h"
#include "DescriptorPool.h"
#include "Logger.h"

bool BindComputePipeline(ID3D12GraphicsCommandList* pCmdList, const std::wstring& pipelineName, DescriptorPool* pPool)
{
    if (!pCmdList || pipelineName.empty() || !pPool || !pPool->GetHeap())
        return false;

    // パイプラインの取得
    PipelineStateInfo* pInfo = PipelineStateManager::GetInstance().GetPipelineState(pipelineName);
    if (!pInfo || !pInfo->isValid || !pInfo->pPSO.Get())
    {
        ELOG("Compute pipeline not found or invalid: %ls", pipelineName.c_str());
        return false;
    }

	// ディスクリプタヒープの設定
    ID3D12DescriptorHeap* const pHeaps[] = { pPool->GetHeap() };
    pCmdList->SetDescriptorHeaps(1, pHeaps);
    pCmdList->SetPipelineState(pInfo->pPSO.Get());
    pCmdList->SetComputeRootSignature(pInfo->rootSig.GetPtr());
    return true;
}
