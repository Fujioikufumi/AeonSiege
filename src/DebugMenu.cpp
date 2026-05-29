#ifdef _DEBUG
#include "DebugMenu.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "Input.h"
#include "DirectX.h"
#include <algorithm>
#include <stack>
#include <fstream>

namespace debug {

// static宣言
Item*						Item::dummy = nullptr;
Window						Window::dummy;
std::vector<Window>			Menu::m_windows;
std::vector<Menu::SaveData> Menu::m_data;

//=====================================================
/*
* @brief 項目表示
*/
class ItemValue : public Item
{
public:
	ItemValue() : value{}, save(false) {}
	~ItemValue() {}
public:
	Value	value;
	bool	save;
};
/*
* @brief 表示アイテム作成
*/
Item* Item::CreateValue(const char* name, Kind kind, bool isSave)
{
	ItemValue* item = new ItemValue;
	item->name = name;
	item->kind = kind;
	item->save = isSave;
	item->value.color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	return item;
}

//=====================================================
/*
* @brief 変数と紐づけた項目
*/
class ItemBind : public Item
{
public:
	ItemBind() : ptr(nullptr) {}
	~ItemBind() {}
public:
	void* ptr;
};
/*
* @brief 紐づけ項目の作成
*/
Item* Item::CreateBind(const char* name, Kind kind, void* ptr)
{
	ItemBind* item = new ItemBind;
	item->name = name;
	item->kind = kind;
	item->ptr = ptr;
	return item;
}

//=====================================================
/*
* @brief 値変更呼び出し
*/
class ItemCallback : public Item
{
public:
	ItemCallback() {}
	~ItemCallback() {}
public:
	Value		value;
	Callback	func;
};
/*
* @brief 値変更呼び出し作成
*/
Item* Item::CreateCallBack(const char* name, Kind kind, Callback func)
{
	ItemCallback* item = new ItemCallback;
	item->name = name;
	item->kind = kind;
	item->func = func;
	item->value.color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	return item;
}

//=====================================================
/*
* @brief グループ表示
*/
class ItemGroup : public Item
{
public:
	ItemGroup() {}
	~ItemGroup()
	{
		auto it = group.begin();
		while (it != group.end())
		{
			delete (*it);
			++it;
		}
	}
public:
	Items group;
};
/*
* @brief リスト表示の作成
*/
Item* Item::CreateGroup(const char* name)
{
	ItemGroup* item = new ItemGroup;
	item->name = name;
	item->kind = Item::Group;
	return item;
}

//=====================================================
/*
* @brief リスト表示
*/
class ItemList : public Item
{
public:
	ItemList() : selectNo(-1), save(false) {}
	~ItemList() {}
public:
	std::list<std::string>	list;		// アイテム一覧
	int						selectNo;	// 選択項目番号
	ConstCallback			func;		// 項目選択時のコールバック
	bool					save;		// 選択番号の保存
};
/*
* @brief リスト作成
*/
Item* Item::CreateList(const char* name, ConstCallback func, bool isSave)
{
	ItemList* item = new ItemList;
	item->name = name;
	item->kind = Item::List;
	item->func = func;
	item->save = isSave;
	return item;
}

//=====================================================
// Item
Item::Item()
{
	name = "None";
	kind = Kind::Label;
}
Item::~Item()
{
}
const char* Item::GetName() const
{
	return name.c_str();
}
Item::Kind Item::GetKind() const
{
	return kind;
}
/*
* @brief Bool項目の取得
*/
bool Item::GetBool() const
{
	if (kind == Bool)
		return reinterpret_cast<const ItemValue*>(this)->value.flg;
	return false;
}
/*
* @brief 整数項目の取得
*/
int Item::GetInt() const
{
	if (kind == Int)
		return reinterpret_cast<const ItemValue*>(this)->value.nValue;
	else if (kind == List)
		return static_cast<const ItemList*>(this)->selectNo;
	return 0;
}
/*
* @brief 小数項目の取得
*/
float Item::GetFloat() const
{
	if (kind == Float)
		return reinterpret_cast<const ItemValue*>(this)->value.fValue;
	return 0.0f;
}
/*
* @brief ベクトル項目の取得
*/
DirectX::XMFLOAT3 Item::GetVector() const
{
	if (kind == Vector)
		return reinterpret_cast<const ItemValue*>(this)->value.vec;
	return DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}
/*
* @brief 色項目の取得
*/
DirectX::XMFLOAT4 Item::GetColor() const
{
	if (kind == Color)
		return static_cast<const ItemValue*>(this)->value.color;
	return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}
/*
* @brief ファイルパス項目の取得
*/
const char* Item::GetStr() const
{
	if(kind == Label || kind == Path)
		return static_cast<const ItemValue*>(this)->value.str;
	return "";
}

/*
* @brief リスト表示項目の追加
*/
void Item::AddListItem(const char* name)
{
	if (kind != List) return;
	reinterpret_cast<ItemList*>(this)->list.push_back(name);
}
/*
* @brief リスト表示項目の削除
*/
void Item::RemoveListItem(const char* name)
{
	if (kind != List) return;
	reinterpret_cast<ItemList*>(this)->list.remove(name);
}

/*
* @brief グループ項目の追加
*/
void Item::AddGroupItem(Item* item)
{
	if (kind != Group) return;
	static_cast<ItemGroup*>(this)->group.push_back(item);
}
/*
* @brief グループ内の特定項目を取得
*/
Item& Item::operator[](const char* name)
{
	// グループ以外の取得はダミーを返却
	if (kind != Group) return *dummy;

	// グループ内のアイテム一覧を取得
	auto items = static_cast<ItemGroup*>(this)->group;

	// アイテム一覧から指定アイテムの探索
	auto it = std::find_if(items.begin(), items.end(),
		[&name](const Item* obj) {
			return strcmp(obj->GetName(), name) == 0;
		});

	// 見つかったアイテムの返却
	if (it != items.end())
		return **it;

	// 見つからなかったらダミーを返却
	return *dummy;
}

/*
* @brief 文字列から種別に変換
*/
Item::Kind Item::StrToKind(std::string str)
{
	if (str == "Label")		return Item::Label;
	if (str == "Bool")		return Item::Bool;
	if (str == "Int")		return Item::Int;
	if (str == "Float")		return Item::Float;
	if (str == "Vector")	return Item::Vector;
	if (str == "Color")		return Item::Color;
	if (str == "Path")		return Item::Path;
	if (str == "Command")	return Item::Command;
	if (str == "Group")		return Item::Group;
	if (str == "List")		return Item::List;
	return Item::Label;
}
/*
* @brief 種別から文字列に変換
*/
std::string Item::KindToStr(Kind kind)
{
	switch (kind) {
	default:
	case Item::Label:		return "Label";
	case Item::Bool:		return "Bool";
	case Item::Int:			return "Int";
	case Item::Float:		return "Float";
	case Item::Vector:		return "Vector";
	case Item::Color:		return "Color";
	case Item::Path:		return "Path";
	case Item::Command:		return "Command";
	case Item::Group:		return "Group";
	case Item::List:		return "List";
	}
}

//=====================================================
// Window
Window::Window()
	: enable(false), name("")
{
}
/*
* @brief ウィンドウ内の表示項目を取得
*/
Item& Window::operator[](const char* name)
{
	auto it = FindItem(name);
	if (it == items.end())
		return *Item::dummy;
	return **it;
}
/*
* @brief ウィンドウへ表示項目を追加
*/
void Window::AddItem(Item* item)
{
	// 既存の項目と同名のアイテムがないか探索
	auto it = FindItem(item->GetName());
	if (it != items.end()) return;

	// 保存データの場合は、追加時に保存済みのデータで上書き
	Menu::DataRead(name + "/", item);

	// 表示項目の追加
	items.push_back(item);
}
/*
* @brief 一覧の削除
*/
void Window::Clear()
{
	auto it = items.begin();
	while (it != items.end())
	{
		delete* it;
		++it;
	}
	items.clear();
}
/*
* @brief アイテムの探索
*/
Items::iterator Window::FindItem(const char* name)
{
	return std::find_if(items.begin(), items.end(),
		[&name](const Item* obj) {
			return strcmp(obj->GetName(), name) == 0;
		});
}

//=====================================================
// Menu
Menu::Menu()
{
}
Menu::~Menu()
{
}
/*
* @brief 初期化
*/
void Menu::Init()
{
	// ImGui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(GetActiveWindow());
	ImGui_ImplDX11_Init(GetDevice(), GetContext());

	// Dummyデータ初期化
	Item::dummy = Item::CreateValue("None", Item::Label);
	
	// 初期ウィンドウの作成
	Window window;
	window.name = "Menu";
	m_windows.push_back(window);

	// ファイル読み出し
	FILE* fp;
	fopen_s(&fp, "Assets/DebugMenu.csv", "rt");
	if (fp) {
		const int size = 1024;
		char line[size];
		m_data.clear();
		while (fgets(line, size, fp) != NULL) {
			// セル内容取得
			std::vector<std::string> cells;
			*strstr(line, "\n") = ',';
			char* start = line;
			char* end = strstr(start, ",");
			do {
				*end = '\0';
				cells.push_back(start);
				start = end + 1;
				end = strstr(start, ",");
			} while (end);
			// データ追加
			SaveData data;
			data.kind = Item::StrToKind(cells[0]);
			data.path = cells[1];
			data.value = cells[2];
			m_data.push_back(data);
		}
		fclose(fp);
	}
}
/*
* @brief 終了処理
*/
void Menu::Uninit()
{
	// ダミーの削除
	delete Item::dummy;

	// 保存データの作成
	std::string data;
	auto it = m_windows.begin();
	while (it != m_windows.end())
	{
		// ウィンドウやグループが階層となるようにパスを作成
		std::string path = it->name + "/";

		// 書き込みが必要なデータを保存
		auto itemIt = it->items.begin();
		while (itemIt != it->items.end())
		{
			DataWrite(data, path, *itemIt);
			++itemIt;
		}

		// ウィンドウのクリア
		it->Clear();
		++it;
	}

	// 保存処理
	FILE* fp;
	fopen_s(&fp, "Assets/DebugMenu.csv", "wt");
	if (fp) {
		fwrite(data.c_str(), data.size(), 1, fp);
		fclose(fp);
	}

	// ImGui終了処理
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
/*
* @brief 更新処理
*/
void Menu::Update()
{
	// デバッグメニューの表示切り替え
	auto& mainWindow = m_windows[0];
	if (IsKeyPress(VK_SHIFT) && IsKeyPress(VK_SPACE)) {
		if (IsKeyTrigger(VK_RETURN))
			mainWindow.enable ^= true;
	}

	// メインウィンドウのチェック状況に応じてほかのウィンドウを表示
	int cnt = 1; // 0はメインウィンドウなので考慮しない
	auto it = mainWindow.items.begin();
	while (it != mainWindow.items.end())
	{
		m_windows[cnt].enable = (*it)->GetBool();
		++it;
		++cnt;
	}
}
/*
* @brief 表示
*/
void Menu::Draw()
{
	// メインウィドウが有効になっていなければ非表示
	if (!m_windows[0].enable)
		return;

	// Imguiの描画準備
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ウィンドウ内の表示項目を描画
	auto windowIt = m_windows.begin();
	while (windowIt != m_windows.end())
	{
		if (windowIt->enable)
		{
			// ウィンドウ内の描画開始
			ImGui::Begin(windowIt->name.c_str());

			// アイテムごとに描画
			auto itemIt = windowIt->items.begin();
			while (itemIt != windowIt->items.end())
			{
				DrawImgui(*itemIt);
				++itemIt;
			}

			// ウィンドウ内の描画終了
			ImGui::End();
		}
		++windowIt;
	}

	// ImGuiの表示
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/*
* @brief ウィンドウの作成
*/
Window& Menu::Create(const char* name)
{
	// ウィンドウを探索してすでにある場合は追加しない
	auto winIt = std::find_if(m_windows.begin(), m_windows.end(),
		[&name](const Window& obj) {
			return strcmp(obj.name.c_str(), name) == 0;
		});
	if (winIt != m_windows.end()) {
		return *winIt;
	}

	// 個別ウィンドウ設定
	Window window;
	window.name = name;
	window.enable = false;
	window.items.clear();
	m_windows.push_back(window);

	// 初期ウィンドウに追加
	Item* label = Item::CreateValue(name, Item::Bool, true);
	DataRead(m_windows[0].name + "/", label);
	m_windows[0].items.push_back(label);

	return m_windows.back();
}
/*
* @brief 既存ウィンドウの取得
*/
Window& Menu::Get(const char* name)
{
	// ウィンドウの探索
	auto winIt = std::find_if(m_windows.begin(), m_windows.end(),
		[&name](const Window& obj) {
			return strcmp(obj.name.c_str(), name) == 0;
		});

	// 見つからなければダミーを返却
	if (winIt == m_windows.end())
		return Window::dummy;

	// 見つかったウィンドウを返却
	return *winIt;
}
/*
* @brief 項目に応じたImguiの表示
*/
void Menu::DrawImgui(Item* item)
{
	// 各要素の取得
	bool isValue	= typeid(ItemValue) == typeid(*item);
	bool isBind		= typeid(ItemBind) == typeid(*item);
	bool isCallback	= typeid(ItemCallback) == typeid(*item);
	ItemValue*		pValue		= static_cast<ItemValue*>(item);
	ItemBind*		pBind		= static_cast<ItemBind*>(item);
	ItemCallback*	pCallback	= static_cast<ItemCallback*>(item);

	switch (item->GetKind())
	{
	// 項目名のみの表示
	case Item::Label:
		if (isBind) {
			char str[256];
			sprintf_s(str, "%s : %s", item->GetName(), reinterpret_cast<char*>(pBind->ptr));
			ImGui::Text("%s", str);
		}
		else
			ImGui::Text("%s", item->GetName());
		break;
	// チェックフラグの表示
	case Item::Bool:
		if (isValue)
			ImGui::Checkbox(item->GetName(), &pValue->value.flg);
		else if (isBind)
			ImGui::Checkbox(item->GetName(), reinterpret_cast<bool*>(pBind->ptr));
		else if (isCallback) {
			pCallback->func(false, &pCallback->value.flg);
			if (ImGui::Checkbox(item->GetName(), &pCallback->value.flg))
				pCallback->func(true, &pCallback->value.flg);
		}
		break;
	// 整数項目の表示
	case Item::Int:
		if(isValue)
			ImGui::InputInt(item->GetName(), &pValue->value.nValue);
		else if(isBind)
			ImGui::InputInt(item->GetName(), reinterpret_cast<int*>(pBind->ptr));
		else if (isCallback) {
			pCallback->func(false, &pCallback->value.nValue);
			if (ImGui::InputInt(item->GetName(), &pCallback->value.nValue))
				pCallback->func(true, &pCallback->value.nValue);
		}
		break;
	// 小数項目の表示
	case Item::Float:
		if(isValue)
			ImGui::InputFloat(item->GetName(), &pValue->value.fValue);
		else if(isBind)
			ImGui::InputFloat(item->GetName(), reinterpret_cast<float*>(pBind->ptr));
		else if (isCallback) {
			pCallback->func(false, &pCallback->value.fValue);
			if (ImGui::InputFloat(item->GetName(), &pCallback->value.fValue))
				pCallback->func(true, &pCallback->value.fValue);
		}
		break;
	// ベクトル項目の表示
	case Item::Vector:
		if(isValue)
			ImGui::InputFloat3(item->GetName(), &pValue->value.vec.x, "%.2f");
		else if(isBind)
			ImGui::InputFloat3(item->GetName(), reinterpret_cast<float*>(pBind->ptr), "%.2f");
		else if (isCallback) {
			pCallback->func(false, &pCallback->value.vec.x);
			if (ImGui::InputFloat3(item->GetName(), &pCallback->value.vec.x, "%.2f"))
				pCallback->func(true, &pCallback->value.vec.x);
		}
		break;
	// 色項目の表示
	case Item::Color:
		if (isValue)
			ImGui::ColorEdit4(item->GetName(), &pValue->value.color.x);
		else if(isBind)
			ImGui::ColorEdit4(item->GetName(), reinterpret_cast<float*>(pBind->ptr));
		else if (isCallback) {
			pCallback->func(false, &pCallback->value.color.x);
			if(ImGui::ColorEdit4(item->GetName(), &pCallback->value.color.x))
				pCallback->func(true, &pCallback->value.color.x);
		}
		break;
	// パス項目の表示
	case Item::Path:
		if (typeid(ItemValue) == typeid(*item)) // 通常表示
			ImGui::InputText(item->GetName(), static_cast<ItemValue*>(item)->value.str, MAX_PATH);
		else // 紐づけ項目の表示
			ImGui::InputText(item->GetName(), reinterpret_cast<char*>(static_cast<ItemBind*>(item)->ptr), MAX_PATH);
		break;
	// ボタンの表示
	case Item::Kind::Command:
		if (ImGui::Button(item->GetName()))
			pCallback->func(false, nullptr);
		break;
	// グループ項目の表示
	case Item::Group:
		{
			ItemGroup* ptr = reinterpret_cast<ItemGroup*>(item);
			// 表示項目がなければ表示しない
			if (ptr->group.empty()) break;

			// グループが展開されてなければ表示しない
			if (!ImGui::CollapsingHeader(item->GetName(), ImGuiTreeNodeFlags_DefaultOpen)) break;

			// グループ内の項目を再帰で表示
			auto it = ptr->group.begin();
			while (it != ptr->group.end())
			{
				DrawImgui(*it);
				++it;
			}
		}
		break;
	// リスト項目の表示
	case Item::List:
		{
			ItemList* ptr = reinterpret_cast<ItemList*>(item);
			// 表示項目がなければ表示しない
			if (ptr->list.empty()) break;

			// 表示項目の構築
			static const char* pList[100];
			auto it = ptr->list.begin();
			for (int i = 0; i < ptr->list.size() && i < 100; ++i, ++it)
				pList[i] = it->c_str();

			// 表示
			if(ImGui::ListBox(item->GetName(), &ptr->selectNo, pList, static_cast<int>(ptr->list.size())))
			{
				// 関数があれば選択時処理を実行
				if(ptr->func && ptr->selectNo >= 0)
					ptr->func(pList[ptr->selectNo]);
			}
		}
		break;
	}
}
/*
* @brief データの保存
*/
void Menu::DataWrite(std::string& data, std::string path, Item* item)
{
	// グループアイテムであれば、パスを更新して再帰呼び出し
	if (item->GetKind() == Item::Group)
	{
		ItemGroup* groupItem= static_cast<ItemGroup*>(item);

		// パスの更新
		path = path + groupItem->GetName() + "/";

		// グループ内アイテムのデータ書き込み
		auto it = groupItem->group.begin();
		while (it != groupItem->group.end())
		{
			DataWrite(data, path, *it);
			++it;
		}

		// 以降の処理は行わない
		return;
	}

	// 保存フラグが設定されているか確認
	ItemValue* pValue = static_cast<ItemValue*>(item);
	ItemList* pList = static_cast<ItemList*>(item);
	if (typeid(*item) == typeid(ItemValue)) {
		if (!pValue->save) return;
	}
	else if (typeid(*item) == typeid(ItemList)) {
		if (!pList->save) return;
	}
	else {
		return;
	}

	// 項目の書き込み
	data += Item::KindToStr(item->GetKind()) + ",";
	// 表示名の書き込み
	data += path + item->GetName() + ",";
	// 項目別の書き込み
	switch (item->GetKind())
	{
	default:
		data += "0";
	case Item::Bool:
		data += pValue->value.flg ? "1" : "0";
		break;
	case Item::Int:
		data += std::to_string(pValue->value.nValue);
		break;
	case Item::Float:
		data += std::to_string(pValue->value.fValue);
		break;
	case Item::Vector:
		data += std::to_string(pValue->value.vec.x) + "/";
		data += std::to_string(pValue->value.vec.y) + "/";
		data += std::to_string(pValue->value.vec.z);
		break;
	case Item::Color:
		data += std::to_string(pValue->value.color.x) + "/";
		data += std::to_string(pValue->value.color.y) + "/";
		data += std::to_string(pValue->value.color.z) + "/";
		data += std::to_string(pValue->value.color.w);
		break;
	case Item::List:
		data += std::to_string(pList->selectNo);
		break;
	}
	data += "\n";
}
/*
* @brief データの読み取り
*/
void Menu::DataRead(std::string path, Item* item)
{
	// グループアイテムであれば、パスを更新して再帰呼び出し
	if (item->GetKind() == Item::Group)
	{
		ItemGroup* groupItem = static_cast<ItemGroup*>(item);
		// パスの更新
		path = path + groupItem->GetName() + "/";

		// グループ内アイテムのデータ読み取り
		auto it = groupItem->group.begin();
		while (it != groupItem->group.end())
		{
			DataRead(path, *it);
			++it;
		}

		// 以降の処理は行わない
		return;
	}

	// 保存フラグが設定されているか確認
	ItemValue* pValue = static_cast<ItemValue*>(item);
	ItemList* pList = static_cast<ItemList*>(item);
	if (typeid(*item) == typeid(ItemValue)) {
		if (!pValue->save) return;
	}
	else if (typeid(*item) == typeid(ItemList)) {
		if (!pList->save) return;
	}
	else {
		return;
	}

	// パスと項目名から読み取るデータ名を作成
	path += item->GetName();

	// データ名に対応するデータを探索
	auto dataIt = std::find_if(m_data.begin(), m_data.end(),
		[&path](const SaveData& data) {
			return data.path == path;
		});

	// なければデータの上書きを行わない
	if (dataIt == m_data.end()) return;

	// 項目に応じてデータの読み取り
	switch (item->GetKind())
	{
	case Item::Bool:
		pValue->value.flg = atoi(dataIt->value.c_str()) > 0;
		break;
	case Item::Int:
		pValue->value.nValue = atoi(dataIt->value.c_str());
		break;
	case Item::Float:
		pValue->value.fValue = strtof(dataIt->value.c_str(), nullptr);
		break;
	case Item::Vector: {
		const char* top[] = {
			dataIt->value.c_str(),
			strstr(top[0], "/") + 1,
			strstr(top[1], "/") + 1,
		};
		pValue->value.vec = DirectX::XMFLOAT3(
			strtof(top[0], nullptr),
			strtof(top[1], nullptr),
			strtof(top[2], nullptr)
		);
	} break;
	case Item::Color: {
		const char* top[] = {
			dataIt->value.c_str(),
			strstr(top[0], "/") + 1,
			strstr(top[1], "/") + 1,
			strstr(top[2], "/") + 1,
		};
		pValue->value.color = DirectX::XMFLOAT4(
			strtof(top[0], nullptr),
			strtof(top[1], nullptr),
			strtof(top[2], nullptr),
			strtof(top[3], nullptr)
		);
		} break;
	case Item::List:
		pList->selectNo = atoi(dataIt->value.c_str());
		if (pList->func) {
			auto it = pList->list.begin();
			for (int i = 0; i < pList->selectNo; ++i)
				it++;
			pList->func(it->c_str());
		}
		break;
	}
}
} // namespace debug
#endif // _DEBUG