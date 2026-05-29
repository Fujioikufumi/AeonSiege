#include "ImGuiUtil.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
//      コンストラクタ
//-----------------------------------------------------------------------------
ImGuiUtil::ImGuiUtil()
{
}

//-----------------------------------------------------------------------------
//	  デストラクタ
//-----------------------------------------------------------------------------
ImGuiUtil::~ImGuiUtil()
{
}

//-----------------------------------------------------------------------------
//	  初期化処理
//-----------------------------------------------------------------------------
bool ImGuiUtil::Init(HWND hWnd, ID3D12Device* pDevice, uint32_t frameCount, DXGI_FORMAT rtvFormat)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // キーボード操作を有効化
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッド操作を有効化

	ImGui::StyleColorsDark(); // ダークテーマを設定

	// ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(pDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap))))
	{
		ELOG("Failed to create descriptor heap for ImGui.");
		return false;
	}

	// ImGuiの初期化
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX12_Init(
		pDevice, 
		frameCount,
		rtvFormat,
		m_srvHeap.Get(),
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_srvHeap->GetGPUDescriptorHandleForHeapStart()
	);

	// フォントアトラスを明示的にビルド
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	m_Initalized = true;
	DLOG("ImGui initialized successfully.");
	return true;
}

//-----------------------------------------------------------------------------
//	  終了処理
//-----------------------------------------------------------------------------
void ImGuiUtil::Term()
{
	if (!m_Initalized) return;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_srvHeap.Reset();
	m_Initalized = false;
	DLOG("ImGui terminated successfully.");
}

//-----------------------------------------------------------------------------
//	  フレームの先頭で呼ぶ
//-----------------------------------------------------------------------------
void ImGuiUtil::NewFrame()
{
	if (!m_Initalized) return;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiUtil::Render(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_Initalized) return;

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();

	if (drawData == nullptr || drawData->CmdListsCount == 0)
	{
		return; // 描画データがない場合は何もしない
	}

	// ImGui用のディスクリプタヒープを設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
	pCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// imGuiの描画
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);
}