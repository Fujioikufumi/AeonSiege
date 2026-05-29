#include "DebugUI.h"
#include "Logger.h"
#include "Input.h"
#include "AnimationController.h"
#include "MeshRenderer.h"
#include "Terrain.h"
#include "CombatHud.h"
#include "CombatHudSerializer.h"
#include <cstdio>
#include "FileUtil.h"
#include <filesystem>

/// <summary>
/// レイヤー名をchar*で取得
/// </summary>
static const char* GetLayerName(eLayer layer)
{
	switch (layer)
	{
	case eLayer::DEFAULT:    return "DEFAULT";
	case eLayer::PLAYER:     return "PLAYER";
	case eLayer::ALLY:       return "ALLY";
	case eLayer::ENEMY:      return "ENEMY";
	case eLayer::BULLET:     return "BULLET";
	case eLayer::ITEM:       return "ITEM";
	case eLayer::UI:         return "UI";
	case eLayer::EFFECT:    return "EFFECT";
	case eLayer::BACKGROUND: return "BACKGROUND";
	case eLayer::WALL:       return "WALL";
	case eLayer::TERRAIN:    return "TERRAIN";
	default:                 return "Unknown";
	}
}

namespace {

	// 選択ポインタがまだシーンのリストに載っているか（削除済みのポインタを弾く）
	bool IsGameObjectInScene(Scene* scene, GameObject* target)
	{
		if (scene == nullptr || target == nullptr)
			return false;
		for (int li = 0; li < static_cast<int>(eLayer::MAX); ++li)
		{
			const eLayer layer = static_cast<eLayer>(li);
			for (const auto& up : scene->GetGameObjectsByLayer(layer))
			{
				if (up.get() == target)
					return true;
			}
		}
		return false;
	}

	// オブジェクトのパラメータ＋コンポーネントのデバッグ UI（詳細ウィンドウ用）
	void DrawGameObjectDetailBody(GameObject* pObj)
	{
		if (pObj == nullptr)
			return;

		// 名前
		std::string nameStr = pObj->GetObjName();
		char nameBuf[256];
		strncpy_s(nameBuf, nameStr.c_str(), _TRUNCATE);
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			pObj->SetObjName(std::string(nameBuf));
		}

		// Position
		DirectX::XMFLOAT3 pos = pObj->GetPosition();
		float posF[3] = { pos.x, pos.y, pos.z };
		if (ImGui::InputFloat3("Position", posF, "%.3f"))
		{
			pObj->SetPosition(DirectX::XMFLOAT3(posF[0], posF[1], posF[2]));
		}

		// Rotation（度で表示）
		const float radToDeg = 180.0f / DirectX::XM_PI;
		const float degToRad = DirectX::XM_PI / 180.0f;

		DirectX::XMFLOAT3 rotRad = pObj->GetRotation();
		float rotDeg[3] = {
			rotRad.x * radToDeg,
			rotRad.y * radToDeg,
			rotRad.z * radToDeg
		};
		if (ImGui::InputFloat3("Rotation (deg)", rotDeg, "%.2f"))
		{
			pObj->SetRotation(DirectX::XMFLOAT3(
				rotDeg[0] * degToRad,
				rotDeg[1] * degToRad,
				rotDeg[2] * degToRad
			));
		}

		// Scale
		DirectX::XMFLOAT3 scl = pObj->GetScale();
		float sclF[3] = { scl.x, scl.y, scl.z };
		if (ImGui::InputFloat3("Scale", sclF, "%.3f"))
		{
			pObj->SetScale(DirectX::XMFLOAT3(sclF[0], sclF[1], sclF[2]));
		}

		// Layer
		int layerIndex = static_cast<int>(pObj->GetLayer());
		const char* layerNames[] = {
			"BACKGROUND",
			"TERRAIN",
			"Environment",
			"EFFECT",
			"DEFAULT",
			"PLAYER",
			"ALLY",
			"ENEMY",
			"BULLET",
			"PLAYER_BULLET",
			"ENEMY_BULLET",
			"WALL",
			"ITEM",
			"UI",
		};
		const int layerCount = static_cast<int>(sizeof(layerNames) / sizeof(layerNames[0]));
		// または static_cast<int>(eLayer::MAX);  // MAX が 14 で「有効レイヤー数」と一致している場合
		if (ImGui::Combo("Layer", &layerIndex, layerNames, layerCount))
		{
			if (layerIndex >= 0 && layerIndex < layerCount)
				pObj->SetLayer(static_cast<eLayer>(layerIndex));
		}

		ImGui::Separator();
		ImGui::Text("Type: %s", typeid(*pObj).name());

		ImGui::Separator();
		ImGui::TextUnformatted("Components:");

		for (const auto& comp : pObj->GetComponents())
		{
			if (ImGui::CollapsingHeader(comp->GetComponentName().c_str()))
			{
				ImGui::PushID(comp.get());
				comp->OnDebugWindow();
				ImGui::PopID();
			}
		}
	}

} // namespace

//-----------------------------------------------------------------------------
//   コンストラクタ
//-----------------------------------------------------------------------------
DebugUI::DebugUI()
	: m_Visible(false) // デフォルトは非表示
{
	m_windows.resize(static_cast<size_t>(DebugWindowType::Max));
	m_windows[static_cast<size_t>(DebugWindowType::Performance)]	= { "Performance",       true,  DebugWindowType::Performance };
	m_windows[static_cast<size_t>(DebugWindowType::Hierarchy)]		= { "Hierarchy",         false, DebugWindowType::Hierarchy };
	m_windows[static_cast<size_t>(DebugWindowType::Inspector)]		= { "Inspector",         true, DebugWindowType::Inspector };
	m_windows[static_cast<size_t>(DebugWindowType::ImGuiDemo)]		= { "ImGui Demo",        false, DebugWindowType::ImGuiDemo };
	m_windows[static_cast<size_t>(DebugWindowType::ImGuiMetrics)]	= { "ImGui Metrics",     false, DebugWindowType::ImGuiMetrics };
	m_windows[static_cast<size_t>(DebugWindowType::CombatHudLayout)] = { "Combat HUD Layout", false, DebugWindowType::CombatHudLayout };
	m_windows[static_cast<size_t>(DebugWindowType::InspectorDetail)] = { "Game Object Detail", true, DebugWindowType::InspectorDetail };
}

//-----------------------------------------------------------------------------
//  デストラクタ
//-----------------------------------------------------------------------------
DebugUI::~DebugUI()
{
}

//-----------------------------------------------------------------------------
// 	更新処理
//-----------------------------------------------------------------------------
void DebugUI::Update(float deltaTime, uint32_t frameIndex, uint32_t width, uint32_t height)
{
	// シフト + スペース + エンター で表示・非表示切り替え
	if(IsKeyPress(VK_SHIFT) && IsKeyPress(VK_SPACE) && IsKeyPress(VK_RETURN))
	{
		m_Visible = !m_Visible;
	}

	if (!m_Visible) return;

	DrawMainMenuBar();

	for (auto& w : m_windows)
	{
		if (!w.enable) continue;

		switch (w.type)
		{
		case DebugWindowType::Performance:
			DrawPerformanceWindow(deltaTime, frameIndex, width, height);
			break;
		case DebugWindowType::Hierarchy:
			DrawHierarchyWindow();
			break;
		case DebugWindowType::ImGuiDemo:
			ImGui::ShowDemoWindow(&w.enable);
			break;
		case DebugWindowType::ImGuiMetrics:
			ImGui::ShowMetricsWindow(&w.enable);
			break;
		case DebugWindowType::Inspector:
			DrawInspectorWindow();
			break;
		case DebugWindowType::InspectorDetail:
			DrawInspectorDetailWindow();
			break;
		case DebugWindowType::CombatHudLayout:
			DrawCombatHudLayoutWindow();
			break;
		default:
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// 	描画処理
//-----------------------------------------------------------------------------
void DebugUI::Draw()
{
	// 特に何もしない
}

//-----------------------------------------------------------------------------
// 	    メインメニューバーの描画
//-----------------------------------------------------------------------------
void DebugUI::DrawMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("DebugMenu"))
		{
			for (auto& w : m_windows)
			{
				ImGui::MenuItem(w.name.c_str(), nullptr, &w.enable);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void DebugUI::DrawInspectorDetailWindow()
{
	DebugWindowEntry* pEntry = GetWindowEntry(DebugWindowType::InspectorDetail);
	bool* pOpen = pEntry ? &pEntry->enable : nullptr;
	if (!ImGui::Begin("Game Object Detail", pOpen))
	{
		ImGui::End();
		return;
	}

	if (m_pScene == nullptr)
	{
		ImGui::TextUnformatted("No scene loaded.");
		ImGui::End();
		return;
	}

	if (m_pSelectedGameObject != nullptr &&
		(m_pSelectedGameObject->IsDestroyed() || !IsGameObjectInScene(m_pScene, m_pSelectedGameObject)))
	{
		m_pSelectedGameObject = nullptr;
	}

	if (m_pSelectedGameObject == nullptr)
	{
		ImGui::TextUnformatted("Select an object in the Inspector list.");
		ImGui::End();
		return;
	}

	DrawGameObjectDetailBody(m_pSelectedGameObject);
	ImGui::End();
}

//-----------------------------------------------------------------------------
// 		ヒエラルキーウィンドウの描画
//-----------------------------------------------------------------------------
void DebugUI::DrawHierarchyWindow()
{
	DebugWindowEntry* pEntry = GetWindowEntry(DebugWindowType::Hierarchy);
	bool* pOpen = pEntry ? &pEntry->enable : nullptr;
	if (!ImGui::Begin("Hierarchy", pOpen))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped(
		"Object list: enable \"Inspector\" in DebugMenu.\n"
		"Details / components: enable \"Game Object Detail\".");
	ImGui::End();
}

//-----------------------------------------------------------------------------
// 	インスペクターウィンドウの描画
//-----------------------------------------------------------------------------
void DebugUI::DrawInspectorWindow()
{
	DebugWindowEntry* pEntry = GetWindowEntry(DebugWindowType::Inspector);
	bool* pOpen = pEntry ? &pEntry->enable : nullptr;
	if (!ImGui::Begin("Inspector", pOpen))
	{
		ImGui::End();
		return;
	}

	if (m_pScene == nullptr)
	{
		ImGui::TextUnformatted("No scene loaded.");
		ImGui::End();
		return;
	}

	// 破棄済み／シーン外の選択は解除
	if (m_pSelectedGameObject != nullptr &&
		(m_pSelectedGameObject->IsDestroyed() || !IsGameObjectInScene(m_pScene, m_pSelectedGameObject)))
	{
		m_pSelectedGameObject = nullptr;
	}

	ImGui::TextUnformatted("Scene objects — click to select:");
	ImGui::Separator();

	for (int li = 0; li < static_cast<int>(eLayer::MAX); ++li)
	{
		const eLayer layer = static_cast<eLayer>(li);
		const auto& list = m_pScene->GetGameObjectsByLayer(layer);
		if (list.empty())
			continue;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (ImGui::TreeNodeEx(static_cast<void*>(reinterpret_cast<void*>(static_cast<intptr_t>(li + 1))), flags, "%s (%zu)",
			GetLayerName(layer), list.size()))
		{
			for (const auto& up : list)
			{
				GameObject* obj = up.get();
				if (obj == nullptr || obj->IsDestroyed())
					continue;

				ImGui::PushID(obj);

				const bool selected = (m_pSelectedGameObject == obj);
				std::string label = obj->GetObjName();
				if (label.empty())
					label = "(unnamed)";
				label += "##";
				label += std::to_string(reinterpret_cast<std::uintptr_t>(obj));

				if (ImGui::Selectable(label.c_str(), selected))
					m_pSelectedGameObject = obj;

				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	if (m_pSelectedGameObject != nullptr)
	{
		ImGui::Separator();
		ImGui::Text("Selected: %s", m_pSelectedGameObject->GetObjName().c_str());
	}
	else
	{
		ImGui::Separator();
		ImGui::TextUnformatted("No selection.");
	}

	ImGui::End();
}

//-----------------------------------------------------------------------------
// 	指定種類のウィンドウエントリを取得（メニューや ImGui の閉じる用に enable を渡すため）
//-----------------------------------------------------------------------------
DebugWindowEntry* DebugUI::GetWindowEntry(DebugWindowType type)
{
	size_t idx = static_cast<size_t>(type);
	if (idx >= m_windows.size()) return nullptr;
	return &m_windows[idx];
}

//-----------------------------------------------------------------------------
// 	パフォーマンスウィンドウの描画
//-----------------------------------------------------------------------------
void DebugUI::DrawPerformanceWindow(float deltaTime, uint32_t frameIndex, uint32_t width, uint32_t height)
{
	ImGui::Begin("Performance", &m_ShowPerformanceWindow);

	float fps = (deltaTime > 1e-6f) ? (1.0f / deltaTime) : 0.0f;
	ImGui::Text("FPS: %.2f", fps);
	ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
	ImGui::Text("Frame Index: %d", frameIndex);
	ImGui::Text("Window Size: %d x %d", width, height);

	ImGui::End();
}

void DebugUI::DrawCombatHudLayoutWindow()
{
	DebugWindowEntry* pEntry = GetWindowEntry(DebugWindowType::CombatHudLayout);
	if (pEntry == nullptr) return;
	bool open = pEntry->enable;
	if (!ImGui::Begin("Combat HUD Layout", &open))
	{
		ImGui::End();
		pEntry->enable = open;
		return;
	}
	pEntry->enable = open;

	if (m_pScene == nullptr)
	{
		ImGui::TextUnformatted("Scene is null.");
		ImGui::End();
		return;
	}

	CombatHud* hud = m_pScene->GetGameObjectByName<CombatHud>("CombatHud");
	if (hud == nullptr)
	{
		ImGui::TextUnformatted("CombatHud not found.");
		ImGui::End();
		return;
	}

	// 新しいレイアウトデータ構造を参照
	CombatHudLayout& layout = hud->GetLayout();

	auto mark = [hud]() { hud->RefreshLayout(); };

	auto drag = [&](const char* label, float* v, float vmin, float vmax)
		{
			if (ImGui::SliderFloat(label, v, vmin, vmax))
				mark();
		};

	auto box4 = [&](const char* head, HudBox& b)
		{
			if (!ImGui::TreeNode(head)) return;
			drag("cx", &b.cx, 0.0f, 1.0f);
			drag("cy", &b.cy, 0.0f, 1.0f);
			drag("w", &b.w, 0.01f, 0.6f);
			drag("h", &b.h, 0.005f, 0.5f);
			ImGui::TreePop();
		};

	// 味方パネルごとの編集
	for (size_t i = 0; i < layout.allies.size(); ++i)
	{
		AllyLayout& A = layout.allies[i];
		const std::string h = "[" + std::to_string(i) + "] " + A.hpFrom;
		if (!ImGui::CollapsingHeader(h.c_str())) continue;

		ImGui::PushID(static_cast<int>(i));
		box4("panel", A.panel);

		if (ImGui::TreeNode("HP Section")) {
			box4("barBack", A.hp.barBack);
			if (ImGui::IsItemEdited()) {
				A.hp.bar = A.hp.barBack;
				mark();
			}
			box4("bar", A.hp.bar);
			drag("Num X", &A.hp.number.onesDigitX, 0.0f, 1.0f);
			drag("Num Y", &A.hp.number.onesDigitY, 0.0f, 1.0f);
			drag("Scale", &A.hp.number.scale, 0.1f, 1.0f);
			if (ImGui::ColorEdit3("Back Color", A.hp.colBack.data())) mark();
			if (ImGui::ColorEdit3("Bar Color", A.hp.colBar.data())) mark();
			ImGui::TreePop();
		}

		// レベル / 経験値セクション
		if (ImGui::TreeNode("Level & EXP Layout")) {
			box4("EXP Bar", A.exp.barBack);
			if (ImGui::IsItemEdited()) { A.exp.bar = A.exp.barBack; mark(); }

			ImGui::Separator();
			ImGui::Text("Level Number Positions");
			drag("Lv Num X", &A.levelNumber.onesDigitX, 0.0f, 1.0f);
			drag("Lv Num Y", &A.levelNumber.onesDigitY, 0.0f, 1.0f);
			drag("Lv Scale", &A.levelNumber.scale, 0.05f, 1.0f);

			if (ImGui::ColorEdit3("EXP Color", A.exp.colBar.data())) mark();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	// 全体・エネミー設定
	if (ImGui::CollapsingHeader("Common / Enemy"))
	{
		box4("Enemy Decor", layout.enemyDecor);
		box4("Enemy Bar", layout.enemyBar);
		if (ImGui::ColorEdit3("Gray RGB", layout.colGray.data())) mark();
		if (ImGui::ColorEdit3("HP RGB", layout.colHp.data())) mark();
		if (ImGui::ColorEdit3("HP Lag RGB", layout.colHpLag.data())) mark();

		ImGui::Separator();
		drag("Skill Center X", &layout.skillCenterX, 0.0f, 1.0f);
		drag("Skill Y", &layout.skillY, 0.0f, 1.0f);
		drag("Skill H", &layout.skillH, 0.02f, 0.2f);
		drag("Skill Gap", &layout.skillGap, 0.01f, 0.2f);
	}

	ImGui::Separator();

	// セーブ・リロード処理も Serializer 経由に修正
	const std::wstring jsonPath = L"Assets/AppData/CombatHudLayout.json";

	if (ImGui::Button("Save layout (JSON)"))
	{
		if (CombatHudSerializer::Save(jsonPath, layout))
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved.");
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload layout (JSON)"))
	{
		if (CombatHudSerializer::Load(jsonPath, layout)) {
			mark();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Reloaded.");
		}
	}

	ImGui::End();
}