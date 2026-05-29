/**
 * 2024.04.08
 *	imgui導入、テキストファイル読み込み、バインド追加
 * 2024.04.09
 *	Bind処理をコールバックで対応、データの読み書きを追加、リスト追加、シーンの切り替え
 * 2024.04.10
 *	モデルの切り替え、シーンの切り替え
 * 2024.07.23
 * 　表示内容をデータから読み取るのではなく、プログラム上で設定できるように変更
 * 　GameObjectを変更して、Unity風のインスペクター表示を行うように変更
 * 2024.11.20
 * 　Labelアイテムに項目名とは別に文字列を設定できるように変更
 * 　アイテムから文字列を取得できるように変更
 */
#ifndef __DEBUG_MENU_H__
#define __DEBUG_MENU_H__

#ifdef _DEBUG
#include <string>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <vector>
#include <DirectXMath.h>
#include <Windows.h>

namespace debug {

/*
* @brief 表示項目
*/
class Item
{
public:
	using Callback = std::function<void(bool isWrite, void* arg)>;
	using ConstCallback = std::function<void(const void* arg)>;
	union Value {
		bool				flg;
		int					nValue;
		float				fValue;
		DirectX::XMFLOAT3	vec;
		DirectX::XMFLOAT4	color;
		char				str[MAX_PATH];
	};
	enum Kind {
		Label,		// 項目のみの表示
		Bool,		// チェックボックス
		Int,		// 整数入力
		Float,		// 小数入力
		Vector,		// ベクター入力
		Color,		// 色入力
		Path,		// ファイルパスの指定
		Command,	// ボタン
		Group,		// 表示項目をまとめる
		List,		// 一覧表示
	};
protected:
	Item();
public:
	virtual ~Item();
	// 項目追加
	static Item* CreateValue(const char* name, Kind kind, bool isSave = false);
	// 変数を紐づけた項目
	static Item* CreateBind(const char* name, Kind kind, void* ptr);
	// ボタンor値変更時
	static Item* CreateCallBack(const char* name, Kind kind, Callback func);
	// グループ
	static Item* CreateGroup(const char* name);
	// 一覧表示
	static Item* CreateList(const char* name, ConstCallback func = nullptr, bool isSave = false);

public:
	// 項目名の取得
	const char* GetName() const;
	// 項目種別の取得
	Kind GetKind() const;

	// 各種設定値の取得
	bool GetBool() const;
	int GetInt() const;
	float GetFloat() const;
	DirectX::XMFLOAT3 GetVector() const;
	DirectX::XMFLOAT4 GetColor() const;
	const char* GetStr() const;

	// グループの設定
	void AddGroupItem(Item* item);
	Item& operator[](const char* name);

	// 一覧表示の設定
	void AddListItem(const char* name);
	void RemoveListItem(const char* name);

public:
	// 文字列から項目の種別を取得
	static Kind StrToKind(std::string str);
	// 項目の種別から文字列を取得
	static std::string KindToStr(Kind kind);

public:
	static Item* dummy;	// 指定データが取得できなかった場合のダミー
private:
	std::string name;	// 
	Kind kind;			// 項目種別
};
using Items = std::list<Item*>;

/*
* @brief デバッグウィンドウ
*/
class Window
{
	friend class Menu;
public:
	Window();
	Item& operator[](const char* name);

	// アイテム追加
	void AddItem(Item* item);
	// アイテム削除
	void RemoveItem(const char* name);
	// 全アイテム削除
	void Clear();

private:
	// アイテムの探索
	Items::iterator FindItem(const char* name);

public:
	static Window	dummy;	// 指定ウィンドウが取得できなかったときのダミー
private:
	bool			enable;	// 表示フラグ
	std::string		name;	// ウィンドウ名
	Items			items;	// 表示項目
};

/*
* @brief デバッグ表示一元管理
*/
class Menu
{
	friend class Window;
public:
	// 保存データ
	struct SaveData
	{
		Item::Kind kind;
		std::string path;
		std::string value;
	};
private:
	Menu();
	~Menu();

public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();

	// ウィンドウ生成
	static Window& Create(const char* name);
	// ウィンドウ取得
	static Window& Get(const char* name);
private:
	static void DrawImgui(Item* item);
	static void DataWrite(std::string& data, std::string path, Item* item);
	static void DataRead(std::string path, Item* item);

private:
	static std::vector<Window>		m_windows;	// 表示ウィンドウ一覧
	static std::vector<SaveData>	m_data;		// 保存データ
};

} // namespace debug
#endif // _DEBUG
#endif // __DEBUG_MENU_H__