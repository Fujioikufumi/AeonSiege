#include "Input.h"
#include <Windows.h>
#include <Xinput.h>
#include <cstring>
#include <cmath>
#pragma comment(lib, "xinput.lib")

namespace {

	constexpr DWORD kXInputUserIndex = 0;

	// デッドゾーン外のスティック値だけ 0〜1 風に縮める（方向は呼び出し側で符号を見る）
	float NormalizeStickAxis(float value, float deadzone)
	{
		float a = fabsf(value);
		if (a <= deadzone)
			return 0.0f;
		float t = (a - deadzone) / (STICK_FORCE_MAX - deadzone);
		if (t > 1.0f) t = 1.0f;
		return (value >= 0.0f) ? t : -t;
	}

	void ApplyThumbDeadzones(XINPUT_STATE& st)
	{
		if ((st.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
			st.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
			(st.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
				st.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			st.Gamepad.sThumbLX = 0;
			st.Gamepad.sThumbLY = 0;
		}
		if ((st.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
			st.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
			(st.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
				st.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE))
		{
			st.Gamepad.sThumbRX = 0;
			st.Gamepad.sThumbRY = 0;
		}
	}

} // namespace

BYTE g_keyTable[256];
BYTE g_oldTable[256];

static MouseState g_mouseState = {};
static MouseState g_oldMouseState = {};
static bool g_mouseInitialized = false;
static HWND g_hWnd = nullptr;
static bool g_mouseLocked = false;

static XINPUT_STATE g_padState = {};
static XINPUT_STATE g_padOldState = {};
static bool g_padConnected = false;
static XINPUT_VIBRATION g_vibration = {};
static bool g_keyBind = false;

HRESULT InitInput()
{
	ZeroMemory(g_keyTable, sizeof(g_keyTable));
	ZeroMemory(g_oldTable, sizeof(g_oldTable));
	GetKeyboardState(g_keyTable);

	ZeroMemory(&g_padState, sizeof(g_padState));
	ZeroMemory(&g_padOldState, sizeof(g_padOldState));
	g_padConnected = (XInputGetState(kXInputUserIndex, &g_padState) == ERROR_SUCCESS);
	g_padOldState = g_padState;
	ApplyThumbDeadzones(g_padState);

	ZeroMemory(&g_vibration, sizeof(g_vibration));

	g_mouseInitialized = true;
	POINT cursorPos{};
	GetCursorPos(&cursorPos);
	g_mouseState.x = cursorPos.x;
	g_mouseState.y = cursorPos.y;
	g_oldMouseState = g_mouseState;

	return S_OK;
}

void UninitInput()
{
	StopVibration(static_cast<int>(kXInputUserIndex));

	if (g_mouseLocked)
	{
		ReleaseCapture();
		ShowCursor(TRUE);
		g_mouseLocked = false;
	}
}

void SetInputWindow(HWND hWnd)
{
	g_hWnd = hWnd;
}

void LockMouse(bool lock)
{
	if (g_hWnd == nullptr) return;

	if (lock && !g_mouseLocked)
	{
		SetCapture(g_hWnd);
		ShowCursor(FALSE);
		g_mouseLocked = true;

		RECT rect{};
		GetClientRect(g_hWnd, &rect);
		POINT center{};
		center.x = rect.left + (rect.right - rect.left) / 2;
		center.y = rect.top + (rect.bottom - rect.top) / 2;
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);

		GetCursorPos(&center);
		g_mouseState.x = center.x;
		g_mouseState.y = center.y;
		g_oldMouseState = g_mouseState;
	}
	else if (!lock && g_mouseLocked)
	{
		ReleaseCapture();
		ShowCursor(TRUE);
		g_mouseLocked = false;
	}
}

void UpdateInput()
{
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	GetKeyboardState(g_keyTable);

	// コントローラ: 前フレーム保存 → 取得（未接続時はボタンを押していない状態にする）
	g_padOldState = g_padState;
	DWORD xret = XInputGetState(kXInputUserIndex, &g_padState);
	g_padConnected = (xret == ERROR_SUCCESS);
	if (!g_padConnected)
	{
		ZeroMemory(&g_padState, sizeof(g_padState));
	}
	ApplyThumbDeadzones(g_padState);

	if (g_mouseInitialized && g_mouseLocked && g_hWnd != nullptr)
	{
		g_oldMouseState = g_mouseState;

		POINT cursorPos{};
		GetCursorPos(&cursorPos);
		g_mouseState.x = cursorPos.x;
		g_mouseState.y = cursorPos.y;

		g_mouseState.deltaX = g_mouseState.x - g_oldMouseState.x;
		g_mouseState.deltaY = g_mouseState.y - g_oldMouseState.y;

		RECT rect{};
		GetClientRect(g_hWnd, &rect);
		POINT center{};
		center.x = rect.left + (rect.right - rect.left) / 2;
		center.y = rect.top + (rect.bottom - rect.top) / 2;
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);

		g_mouseState.x = center.x;
		g_mouseState.y = center.y;
	}
	else if (g_mouseInitialized)
	{
		g_oldMouseState = g_mouseState;

		POINT cursorPos{};
		GetCursorPos(&cursorPos);
		g_mouseState.x = cursorPos.x;
		g_mouseState.y = cursorPos.y;

		g_mouseState.deltaX = g_mouseState.x - g_oldMouseState.x;
		g_mouseState.deltaY = g_mouseState.y - g_oldMouseState.y;
	}

	g_mouseState.leftButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	g_mouseState.rightButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	g_mouseState.middleButton = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
}

static bool InputBlockedByBind()
{
	return g_keyBind;
}

bool IsKeyPress(BYTE key)
{
	if (InputBlockedByBind()) return false;
	return (g_keyTable[key] & 0x80) != 0;
}

bool IsKeyTrigger(BYTE key)
{
	if (InputBlockedByBind()) return false;
	return ((g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80) != 0;
}

bool IsKeyRelease(BYTE key)
{
	if (InputBlockedByBind()) return false;
	return ((g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80) != 0;
}

bool IsKeyRepeat(BYTE key)
{
	if (InputBlockedByBind()) return false;
	// OS のキーリピートではなく「連続押下中」検出
	return (g_keyTable[key] & g_oldTable[key] & 0x80) != 0;
}

MouseState GetMouseState()
{
	return g_mouseState;
}

bool IsMouseButtonPress(int button)
{
	switch (button)
	{
	case 0: return g_mouseState.leftButton;
	case 1: return g_mouseState.rightButton;
	case 2: return g_mouseState.middleButton;
	default: return false;
	}
}

bool IsMouseButtonTrigger(int button)
{
	switch (button)
	{
	case 0: return g_mouseState.leftButton && !g_oldMouseState.leftButton;
	case 1: return g_mouseState.rightButton && !g_oldMouseState.rightButton;
	case 2: return g_mouseState.middleButton && !g_oldMouseState.middleButton;
	default: return false;
	}
}

bool IsMouseButtonRelease(int button)
{
	switch (button)
	{
	case 0: return !g_mouseState.leftButton && g_oldMouseState.leftButton;
	case 1: return !g_mouseState.rightButton && g_oldMouseState.rightButton;
	case 2: return !g_mouseState.middleButton && g_oldMouseState.middleButton;
	default: return false;
	}
}

bool IsControllerPress(WORD button)
{
	if (InputBlockedByBind()) return false;
	return (g_padState.Gamepad.wButtons & button) != 0;
}

bool IsControllerTrigger(WORD button)
{
	if (InputBlockedByBind()) return false;
	WORD cur = g_padState.Gamepad.wButtons;
	WORD old = g_padOldState.Gamepad.wButtons;
	return ((cur ^ old) & cur & button) != 0;
}

bool IsControllerRelease(WORD button)
{
	if (InputBlockedByBind()) return false;
	WORD cur = g_padState.Gamepad.wButtons;
	WORD old = g_padOldState.Gamepad.wButtons;
	return ((cur ^ old) & old & button) != 0;
}

bool IsControllerRepeat(WORD button)
{
	if (InputBlockedByBind()) return false;
	return ((g_padState.Gamepad.wButtons & g_padOldState.Gamepad.wButtons) & button) != 0;
}

bool IsLTPress(BYTE strength)
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.bLeftTrigger > strength;
}

bool IsRTPress(BYTE strength)
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.bRightTrigger > strength;
}

bool IsLTTrigger()
{
	if (InputBlockedByBind()) return false;
	BYTE cur = g_padState.Gamepad.bLeftTrigger;
	BYTE old = g_padOldState.Gamepad.bLeftTrigger;
	return (cur > 0 && old == 0);
}

bool IsRTTrigger()
{
	if (InputBlockedByBind()) return false;
	BYTE cur = g_padState.Gamepad.bRightTrigger;
	BYTE old = g_padOldState.Gamepad.bRightTrigger;
	return (cur > 0 && old == 0);
}

bool IsLTRelease()
{
	if (InputBlockedByBind()) return false;
	BYTE cur = g_padState.Gamepad.bLeftTrigger;
	BYTE old = g_padOldState.Gamepad.bLeftTrigger;
	return (cur == 0 && old > 0);
}

bool IsRTRelease()
{
	if (InputBlockedByBind()) return false;
	BYTE cur = g_padState.Gamepad.bRightTrigger;
	BYTE old = g_padOldState.Gamepad.bRightTrigger;
	return (cur == 0 && old > 0);
}

bool IsLLeftStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbLX <= -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

bool IsLRightStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbLX >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

float IsLLeftStickForce()
{
	if (InputBlockedByBind()) return 0.0f;
	float lx = static_cast<float>(g_padState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

float IsLRightStickForce()
{
	if (InputBlockedByBind()) return 0.0f;
	float lx = static_cast<float>(g_padState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

bool IsRLeftStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbRX <= -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

bool IsRRightStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbRX >= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

float IsRLeftStickForce()
{
	if (InputBlockedByBind()) return 0.0f;
	float rx = static_cast<float>(g_padState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

float IsRRightStickForce()
{
	if (InputBlockedByBind()) return 0.0f;
	float rx = static_cast<float>(g_padState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

float GetLeftStickX()
{
	if (InputBlockedByBind()) return 0.0f;
	float lx = static_cast<float>(g_padState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}
float GetLeftStickY()
{
	if (InputBlockedByBind()) return 0.0f;
	float ly = static_cast<float>(g_padState.Gamepad.sThumbLY);
	return NormalizeStickAxis(ly, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}
float GetRightStickX()
{
	if (InputBlockedByBind()) return 0.0f;
	float rx = static_cast<float>(g_padState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}
float GetRightStickY()
{
	if (InputBlockedByBind()) return 0.0f;
	float ry = static_cast<float>(g_padState.Gamepad.sThumbRY);
	return NormalizeStickAxis(ry, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

bool IsLUpStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbLY >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

bool IsLDownStickPress()
{
	if (InputBlockedByBind()) return false;
	return g_padState.Gamepad.sThumbLY <= -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

void Vibration(int controllerNum, int leftMotorSpeed, int rightMotorSpeed)
{
	g_vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotorSpeed);
	g_vibration.wRightMotorSpeed = static_cast<WORD>(rightMotorSpeed);
	XInputSetState(static_cast<DWORD>(controllerNum), &g_vibration);
}

void StopVibration(int controllerNum)
{
	g_vibration.wLeftMotorSpeed = 0;
	g_vibration.wRightMotorSpeed = 0;
	XInputSetState(static_cast<DWORD>(controllerNum), &g_vibration);
}

void KeyBind(bool inBind)
{
	g_keyBind = inBind;
}

bool GetKeyBind()
{
	return g_keyBind;
}

//#include "Input.h"
//#include <Windows.h>
//
////--- グローバル変数
//BYTE g_keyTable[256];
//BYTE g_oldTable[256];
//
//// マウス用グローバル変数
//static MouseState g_mouseState = {};
//static MouseState g_oldMouseState = {};
//static bool g_mouseInitialized = false;
//static HWND g_hWnd = nullptr; // ウィンドウハンドルを保持
//static bool g_mouseLocked = false;
//
//HRESULT InitInput()
//{
//	// 一番最初の入力
//	GetKeyboardState(g_keyTable);
//
//	// マウス初期化
//	g_mouseInitialized = true;
//	POINT cursorPos;
//	GetCursorPos(&cursorPos);
//	g_mouseState.x = cursorPos.x;
//	g_mouseState.y = cursorPos.y;
//	g_oldMouseState = g_mouseState;
//
//	return S_OK;
//}
//void UninitInput()
//{
//	// マウスロックを解除
//	if (g_mouseLocked)
//	{
//		ReleaseCapture();
//		ShowCursor(TRUE);
//		g_mouseLocked = false;
//	}
//}
//
//void SetInputWindow(HWND hWnd)
//{
//	g_hWnd = hWnd;
//}
//
//void LockMouse(bool lock)
//{
//	if (g_hWnd == nullptr) return;
//
//	if (lock && !g_mouseLocked)
//	{
//		// マウスをロック
//		SetCapture(g_hWnd);
//		ShowCursor(FALSE);  // カーソルを非表示
//		g_mouseLocked = true;
//
//		// カーソルを画面中央に移動
//		RECT rect;
//		GetClientRect(g_hWnd, &rect);
//		POINT center;
//		center.x = rect.left + (rect.right - rect.left) / 2;
//		center.y = rect.top + (rect.bottom - rect.top) / 2;
//		ClientToScreen(g_hWnd, &center);
//		SetCursorPos(center.x, center.y);
//
//		// 前フレームの座標をリセット
//		GetCursorPos(&center);
//		g_mouseState.x = center.x;
//		g_mouseState.y = center.y;
//		g_oldMouseState = g_mouseState;
//	}
//	else if (!lock && g_mouseLocked)
//	{
//		// マウスロックを解除
//		ReleaseCapture();
//		ShowCursor(TRUE);  // カーソルを表示
//		g_mouseLocked = false;
//	}
//}
//
//void UpdateInput()
//{
//	// 古い入力を更新
//	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
//	// 現在の入力を取得
//	GetKeyboardState(g_keyTable);
//
//	// マウス状態更新
//	if (g_mouseInitialized && g_mouseLocked && g_hWnd != nullptr)
//	{
//		g_oldMouseState = g_mouseState;
//
//		// マウス座標取得
//		POINT cursorPos;
//		GetCursorPos(&cursorPos);
//		g_mouseState.x = cursorPos.x;
//		g_mouseState.y = cursorPos.y;
//
//		// 移動量計算
//		g_mouseState.deltaX = g_mouseState.x - g_oldMouseState.x;
//		g_mouseState.deltaY = g_mouseState.y - g_oldMouseState.y;
//
//		// カーソルを画面中央に戻す
//		RECT rect;
//		GetClientRect(g_hWnd, &rect);
//		POINT center;
//		center.x = rect.left + (rect.right - rect.left) / 2;
//		center.y = rect.top + (rect.bottom - rect.top) / 2;
//		ClientToScreen(g_hWnd, &center);
//		SetCursorPos(center.x, center.y);
//
//		// 中央座標を更新（次フレームの計算用）
//		g_mouseState.x = center.x;
//		g_mouseState.y = center.y;
//	}
//	else if (g_mouseInitialized)
//	{
//		// ロックされていない場合は通常の処理
//		g_oldMouseState = g_mouseState;
//
//		POINT cursorPos;
//		GetCursorPos(&cursorPos);
//		g_mouseState.x = cursorPos.x;
//		g_mouseState.y = cursorPos.y;
//
//		g_mouseState.deltaX = g_mouseState.x - g_oldMouseState.x;
//		g_mouseState.deltaY = g_mouseState.y - g_oldMouseState.y;
//	}
//
//	// マウスボタン状態取得
//	g_mouseState.leftButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
//	g_mouseState.rightButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
//	g_mouseState.middleButton = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
//}
//
//bool IsKeyPress(BYTE key)
//{
//	return g_keyTable[key] & 0x80;
//}
//bool IsKeyTrigger(BYTE key)
//{
//	return (g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80;
//}
//bool IsKeyRelease(BYTE key)
//{
//	return (g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80;
//}
//bool IsKeyRepeat(BYTE key)
//{
//	return false;
//}
//
//MouseState GetMouseState()
//{
//	return g_mouseState;
//}
//
//bool IsMouseButtonPress(int button)
//{
//	switch (button)
//	{
//	case 0: return g_mouseState.leftButton;
//	case 1: return g_mouseState.rightButton;
//	case 2: return g_mouseState.middleButton;
//	default: return false;
//	}
//}
//
//bool IsMouseButtonTrigger(int button)
//{
//	switch (button)
//	{
//	case 0: return g_mouseState.leftButton && !g_oldMouseState.leftButton;
//	case 1: return g_mouseState.rightButton && !g_oldMouseState.rightButton;
//	case 2: return g_mouseState.middleButton && !g_oldMouseState.middleButton;
//	default: return false;
//	}
//}
//
//bool IsMouseButtonRelease(int button)
//{
//	switch (button)
//	{
//	case 0: return !g_mouseState.leftButton && g_oldMouseState.leftButton;
//	case 1: return !g_mouseState.rightButton && g_oldMouseState.rightButton;
//	case 2: return !g_mouseState.middleButton && g_oldMouseState.middleButton;
//	default: return false;
//	};
//}
