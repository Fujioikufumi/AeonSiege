#include "Input/Input.h"
#include <Xinput.h>
#include <cstring>
#include <cmath>
#pragma comment(lib, "xinput.lib")

namespace {

	constexpr DWORD kXInputUserIndex = 0;

	// デッドゾーン外のスティック値だけ 0〜1 風に縮める（方向は呼び出し側で符号を見る）
	float NormalizeStickAxis(float value, float deadzone)
	{
		float a = fabsf(value); // 絶対値
		if (a <= deadzone) 
			return 0.0f; // デッドゾーン内は 0

		float t = (a - deadzone) / (Input::STICK_FORCE_MAX - deadzone);
		if (t > 1.0f) t = 1.0f;
		return (value >= 0.0f) ? t : -t;
	}

	void ApplyThumbDeadzones(XINPUT_STATE& st)
	{
		if ((st.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  &&
			 st.Gamepad.sThumbLX <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
			(st.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  &&
			 st.Gamepad.sThumbLY <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			st.Gamepad.sThumbLX = 0;
			st.Gamepad.sThumbLY = 0;
		}
		if ((st.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE  &&
			 st.Gamepad.sThumbRX <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
			(st.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE  &&
			 st.Gamepad.sThumbRY <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE))
		{
			st.Gamepad.sThumbRX = 0;
			st.Gamepad.sThumbRY = 0;
		}
	}

} // namespace

//-----------------------------------------------------------------------------
//      初期化
//-----------------------------------------------------------------------------
HRESULT Input::Init()
{
	// キーボード
	ZeroMemory(m_KeyTable, sizeof(m_KeyTable));
	ZeroMemory(m_KeyTablePrev, sizeof(m_KeyTablePrev));
	GetKeyboardState(m_KeyTable);

	// コントローラ
	ZeroMemory(&m_PadState, sizeof(m_PadState));
	ZeroMemory(&m_PadStatePrev, sizeof(m_PadStatePrev));
	m_PadConnected = (XInputGetState(kXInputUserIndex, &m_PadState) == ERROR_SUCCESS);
	m_PadStatePrev = m_PadState;
	ApplyThumbDeadzones(m_PadState);

	ZeroMemory(&m_Vibration, sizeof(m_Vibration));

	// マウス
	m_MouseInitialized = true;
	POINT cursorPos{};
	GetCursorPos(&cursorPos);
	m_MouseState.x = cursorPos.x;
	m_MouseState.y = cursorPos.y;
	m_MouseStatePrev = m_MouseState;

	return S_OK;
}

//-----------------------------------------------------------------------------
//      終了処理
//-----------------------------------------------------------------------------
void Input::Term()
{
	StopVibration(static_cast<int>(kXInputUserIndex));

	if (m_MouseLocked)
	{
		ReleaseCapture();
		ShowCursor(TRUE);
		m_MouseLocked = false;
	}
}

void Input::SetWindow(HWND hWnd)
{
	m_hWnd = hWnd;
}

void Input::LockMouse(bool lock)
{
	if (m_hWnd == nullptr) return;

	if (lock && !m_MouseLocked)
	{
		// マウスキャプチャを開始
		SetCapture(m_hWnd);

		// カーソルを非表示
		ShowCursor(FALSE);

		// マウスロック状態に設定
		m_MouseLocked = true;

		// マウスをウィンドウ中央に移動
		RECT rect{};
		GetClientRect(m_hWnd, &rect);
		POINT center{};
		center.x = rect.left + (rect.right - rect.left) / 2;
		center.y = rect.top + (rect.bottom - rect.top) / 2;

		// クライアント座標をスクリーン座標に変換してカーソル位置を設定
		ClientToScreen(m_hWnd, &center);
		SetCursorPos(center.x, center.y);

		GetCursorPos(&center);
		m_MouseState.x = center.x;
		m_MouseState.y = center.y;
		m_MouseStatePrev = m_MouseState;
	}
	else if (!lock && m_MouseLocked)
	{
		ReleaseCapture();
		ShowCursor(TRUE);
		m_MouseLocked = false;
	}
}

//-----------------------------------------------------------------------------
//      更新（毎フレーム呼ぶ）
//-----------------------------------------------------------------------------
void Input::Update()
{
	memcpy_s(m_KeyTablePrev, sizeof(m_KeyTablePrev), m_KeyTable, sizeof(m_KeyTable));
	GetKeyboardState(m_KeyTable);

	// 前フレームのコントローラ状態を保存
	m_PadStatePrev = m_PadState;

	DWORD xret = XInputGetState(kXInputUserIndex, &m_PadState);
	m_PadConnected = (xret == ERROR_SUCCESS);
	if (!m_PadConnected)
	{
		ZeroMemory(&m_PadState, sizeof(m_PadState));
	}
	// デッドゾーン処理
	ApplyThumbDeadzones(m_PadState);

	// マウス
	if (m_MouseInitialized && m_MouseLocked && m_hWnd != nullptr)
	{
		// 前フレームのマウス状態を保存
		m_MouseStatePrev = m_MouseState;

		POINT cursorPos{};
		GetCursorPos(&cursorPos);
		m_MouseState.x = cursorPos.x;
		m_MouseState.y = cursorPos.y;

		// マウスの移動量を計算
		m_MouseState.deltaX = m_MouseState.x - m_MouseStatePrev.x;
		m_MouseState.deltaY = m_MouseState.y - m_MouseStatePrev.y;

		// マウスをウィンドウ中央に戻す
		RECT rect{};
		GetClientRect(m_hWnd, &rect);
		POINT center{};
		center.x = rect.left + (rect.right - rect.left) / 2;
		center.y = rect.top + (rect.bottom - rect.top) / 2;
		ClientToScreen(m_hWnd, &center);
		SetCursorPos(center.x, center.y);

		m_MouseState.x = center.x;
		m_MouseState.y = center.y;
	}
	else if (m_MouseInitialized)
	{
		m_MouseStatePrev = m_MouseState;

		POINT cursorPos{};
		GetCursorPos(&cursorPos);
		m_MouseState.x = cursorPos.x;
		m_MouseState.y = cursorPos.y;

		m_MouseState.deltaX = m_MouseState.x - m_MouseStatePrev.x;
		m_MouseState.deltaY = m_MouseState.y - m_MouseStatePrev.y;
	}

	m_MouseState.leftButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	m_MouseState.rightButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	m_MouseState.middleButton = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
}

//-----------------------------------------------------------------------------
//      キーボード
//-----------------------------------------------------------------------------
bool Input::IsKeyPress(BYTE key) const
{
	if (m_KeyBind) return false;
	return (m_KeyTable[key] & 0x80) != 0;
}

bool Input::IsKeyTrigger(BYTE key) const
{
	if (m_KeyBind) return false;
	return ((m_KeyTable[key] ^ m_KeyTablePrev[key]) & m_KeyTable[key] & 0x80) != 0;
}

bool Input::IsKeyRelease(BYTE key) const
{
	if (m_KeyBind) return false;
	return ((m_KeyTable[key] ^ m_KeyTablePrev[key]) & m_KeyTablePrev[key] & 0x80) != 0;
}

bool Input::IsKeyRepeat(BYTE key) const
{
	if (m_KeyBind) return false;
	// OS のキーリピートではなく「連続押下中」検出
	return (m_KeyTable[key] & m_KeyTablePrev[key] & 0x80) != 0;
}

//-----------------------------------------------------------------------------
//      マウス
//-----------------------------------------------------------------------------
MouseState Input::GetMouseState() const
{
	return m_MouseState;
}

bool Input::IsMouseButtonPress(int button) const
{
	switch (button)
	{
	case 0: return m_MouseState.leftButton;
	case 1: return m_MouseState.rightButton;
	case 2: return m_MouseState.middleButton;
	default: return false;
	}
}

bool Input::IsMouseButtonTrigger(int button) const
{
	switch (button)
	{
	case 0: return m_MouseState.leftButton && !m_MouseStatePrev.leftButton;
	case 1: return m_MouseState.rightButton && !m_MouseStatePrev.rightButton;
	case 2: return m_MouseState.middleButton && !m_MouseStatePrev.middleButton;
	default: return false;
	}
}

bool Input::IsMouseButtonRelease(int button) const
{
	switch (button)
	{
	case 0: return !m_MouseState.leftButton && m_MouseStatePrev.leftButton;
	case 1: return !m_MouseState.rightButton && m_MouseStatePrev.rightButton;
	case 2: return !m_MouseState.middleButton && m_MouseStatePrev.middleButton;
	default: return false;
	}
}

//-----------------------------------------------------------------------------
//      コントローラ（ボタン）
//-----------------------------------------------------------------------------
bool Input::IsControllerPress(WORD button) const
{
	if (m_KeyBind) return false;
	return (m_PadState.Gamepad.wButtons & button) != 0;
}

bool Input::IsControllerTrigger(WORD button) const
{
	if (m_KeyBind) return false;
	WORD cur = m_PadState.Gamepad.wButtons;
	WORD old = m_PadStatePrev.Gamepad.wButtons;
	return ((cur ^ old) & cur & button) != 0;
}

bool Input::IsControllerRelease(WORD button) const
{
	if (m_KeyBind) return false;
	WORD cur = m_PadState.Gamepad.wButtons;
	WORD old = m_PadStatePrev.Gamepad.wButtons;
	return ((cur ^ old) & old & button) != 0;
}

bool Input::IsControllerRepeat(WORD button) const
{
	if (m_KeyBind) return false;
	return ((m_PadState.Gamepad.wButtons & m_PadStatePrev.Gamepad.wButtons) & button) != 0;
}

//-----------------------------------------------------------------------------
//      コントローラ（トリガー）
//-----------------------------------------------------------------------------
bool Input::IsLTPress(BYTE strength) const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.bLeftTrigger > strength;
}

bool Input::IsRTPress(BYTE strength) const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.bRightTrigger > strength;
}

bool Input::IsLTTrigger() const
{
	if (m_KeyBind) return false;
	BYTE cur = m_PadState.Gamepad.bLeftTrigger;
	BYTE old = m_PadStatePrev.Gamepad.bLeftTrigger;
	return (cur > 0 && old == 0);
}

bool Input::IsRTTrigger() const
{
	if (m_KeyBind) return false;
	BYTE cur = m_PadState.Gamepad.bRightTrigger;
	BYTE old = m_PadStatePrev.Gamepad.bRightTrigger;
	return (cur > 0 && old == 0);
}

bool Input::IsLTRelease() const
{
	if (m_KeyBind) return false;
	BYTE cur = m_PadState.Gamepad.bLeftTrigger;
	BYTE old = m_PadStatePrev.Gamepad.bLeftTrigger;
	return (cur == 0 && old > 0);
}

bool Input::IsRTRelease() const
{
	if (m_KeyBind) return false;
	BYTE cur = m_PadState.Gamepad.bRightTrigger;
	BYTE old = m_PadStatePrev.Gamepad.bRightTrigger;
	return (cur == 0 && old > 0);
}

//-----------------------------------------------------------------------------
//      コントローラ（スティック）
//-----------------------------------------------------------------------------
bool Input::IsLLeftStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbLX <= -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

bool Input::IsLRightStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbLX >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

float Input::IsLLeftStickForce() const
{
	if (m_KeyBind) return 0.0f;
	float lx = static_cast<float>(m_PadState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

float Input::IsLRightStickForce() const
{
	if (m_KeyBind) return 0.0f;
	float lx = static_cast<float>(m_PadState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

bool Input::IsRLeftStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbRX <= -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

bool Input::IsRRightStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbRX >= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
}

float Input::IsRLeftStickForce() const
{
	if (m_KeyBind) return 0.0f;
	float rx = static_cast<float>(m_PadState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

float Input::IsRRightStickForce() const
{
	if (m_KeyBind) return 0.0f;
	float rx = static_cast<float>(m_PadState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

float Input::GetLeftStickX() const
{
	if (m_KeyBind) return 0.0f;
	float lx = static_cast<float>(m_PadState.Gamepad.sThumbLX);
	return NormalizeStickAxis(lx, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

float Input::GetLeftStickY() const
{
	if (m_KeyBind) return 0.0f;
	float ly = static_cast<float>(m_PadState.Gamepad.sThumbLY);
	return NormalizeStickAxis(ly, static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

float Input::GetRightStickX() const
{
	if (m_KeyBind) return 0.0f;
	float rx = static_cast<float>(m_PadState.Gamepad.sThumbRX);
	return NormalizeStickAxis(rx, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

float Input::GetRightStickY() const
{
	if (m_KeyBind) return 0.0f;
	float ry = static_cast<float>(m_PadState.Gamepad.sThumbRY);
	return NormalizeStickAxis(ry, static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
}

bool Input::IsLUpStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbLY >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

bool Input::IsLDownStickPress() const
{
	if (m_KeyBind) return false;
	return m_PadState.Gamepad.sThumbLY <= -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
}

//-----------------------------------------------------------------------------
//      振動
//-----------------------------------------------------------------------------
void Input::Vibration(int controllerNum, int leftMotorSpeed, int rightMotorSpeed)
{
	m_Vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotorSpeed);
	m_Vibration.wRightMotorSpeed = static_cast<WORD>(rightMotorSpeed);
	XInputSetState(static_cast<DWORD>(controllerNum), &m_Vibration);
}

void Input::StopVibration(int controllerNum)
{
	m_Vibration.wLeftMotorSpeed = 0;
	m_Vibration.wRightMotorSpeed = 0;
	XInputSetState(static_cast<DWORD>(controllerNum), &m_Vibration);
}

//-----------------------------------------------------------------------------
//      キーバインド（入力ロック）
//-----------------------------------------------------------------------------
void Input::KeyBind(bool inBind)
{
	m_KeyBind = inBind;
}

bool Input::GetKeyBind() const
{
	return m_KeyBind;
}
