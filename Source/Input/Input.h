#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "Core/SingletonManager.h"

#undef max
#undef min

//-----------------------------------------------------------------------------
// マウス状態（スクリーン座標・ボタン）
//-----------------------------------------------------------------------------
struct MouseState
{
	// マウス座標（スクリーン座標）
	int x = 0;
	int y = 0;

	// マウス移動量（前フレームからの差分）
	int deltaX = 0;
	int deltaY = 0;

	// マウスボタン状態
	bool leftButton		= false;
	bool rightButton	= false;
	bool middleButton	= false;
};

//-----------------------------------------------------------------------------
// 入力管理クラス
//-----------------------------------------------------------------------------
class Input : public SingletonManager<Input>
{
	friend class SingletonManager<Input>;
public:
	// XInput
	static constexpr WORD PAD_UP	= 0x0001;
	static constexpr WORD PAD_DOWN	= 0x0002;
	static constexpr WORD PAD_LEFT	= 0x0004;
	static constexpr WORD PAD_RIGHT = 0x0008;

	static constexpr WORD PAD_START = 0x0010;
	static constexpr WORD PAD_BACK	= 0x0020;
	static constexpr WORD PAD_LEFT_THUMB = 0x0040;
	static constexpr WORD PAD_RIGHT_THUMB = 0x0080;
	static constexpr WORD PAD_LEFT_SHOULDER = 0x0100;
	static constexpr WORD PAD_RIGHT_SHOULDER = 0x0200;

	static constexpr WORD PAD_A = 0x1000;
	static constexpr WORD PAD_B = 0x2000;
	static constexpr WORD PAD_X = 0x4000;
	static constexpr WORD PAD_Y = 0x8000;

	static constexpr float STICK_FORCE_MAX = 32767.0f;
	static constexpr float STICK_FORCE_MIN = -32768.0f;

	// ライフサイクル
	HRESULT Init();
	void    Term();
	void    Update();
	void    SetWindow(HWND hWnd);
	void    LockMouse(bool lock);

	// キーボード
	[[nodiscard]] bool IsKeyPress(BYTE key) const;
	[[nodiscard]] bool IsKeyTrigger(BYTE key) const;
	[[nodiscard]] bool IsKeyRelease(BYTE key) const;
	[[nodiscard]] bool IsKeyRepeat(BYTE key) const;

	// マウス
	[[nodiscard]] MouseState GetMouseState() const;
	[[nodiscard]] bool IsMouseButtonPress(int button) const;
	[[nodiscard]] bool IsMouseButtonTrigger(int button) const;
	[[nodiscard]] bool IsMouseButtonRelease(int button) const;

	// コントローラ（ボタン）
	[[nodiscard]] bool IsControllerPress(WORD button) const;
	[[nodiscard]] bool IsControllerTrigger(WORD button) const;
	[[nodiscard]] bool IsControllerRelease(WORD button) const;
	[[nodiscard]] bool IsControllerRepeat(WORD button) const;

	// コントローラ（トリガー）
	[[nodiscard]] bool IsLTPress(BYTE strength) const;
	[[nodiscard]] bool IsRTPress(BYTE strength) const;
	[[nodiscard]] bool IsLTTrigger() const;
	[[nodiscard]] bool IsRTTrigger() const;
	[[nodiscard]] bool IsLTRelease() const;
	[[nodiscard]] bool IsRTRelease() const;

	// コントローラ（スティック）
	[[nodiscard]] bool  IsLLeftStickPress() const;
	[[nodiscard]] bool  IsLRightStickPress() const;
	[[nodiscard]] float IsLLeftStickForce() const;
	[[nodiscard]] float IsLRightStickForce() const;
	[[nodiscard]] bool  IsRLeftStickPress() const;
	[[nodiscard]] bool  IsRRightStickPress() const;
	[[nodiscard]] float IsRLeftStickForce() const;
	[[nodiscard]] float IsRRightStickForce() const;
	[[nodiscard]] float GetLeftStickX() const;
	[[nodiscard]] float GetLeftStickY() const;
	[[nodiscard]] float GetRightStickX() const;
	[[nodiscard]] float GetRightStickY() const;
	[[nodiscard]] bool  IsLUpStickPress() const;
	[[nodiscard]] bool  IsLDownStickPress() const;

	// 振動
	void Vibration(int controllerNum, int leftMotorSpeed, int rightMotorSpeed);
	void StopVibration(int controllerNum);

	// キーバインド（入力ロック）
	void KeyBind(bool inBind);
	[[nodiscard]] bool GetKeyBind() const;

private:
	Input() = default;
	~Input() = default;

	BYTE m_KeyTable[256]		= {}; // 現在のキー状態
	BYTE m_KeyTablePrev[256]	= {}; // 前フレームのキー状態

	MouseState m_MouseState		= {}; // マウス状態
	MouseState m_MouseStatePrev = {}; // 前フレームのマウス状態

	bool m_MouseInitialized = false; // マウス初期化済みフラグ
	HWND m_hWnd				= nullptr; // 入力対象ウィンドウハンドル
	bool m_MouseLocked = false; // マウスロック状態

	XINPUT_STATE m_PadState{}; // コントローラ状態
	XINPUT_STATE m_PadStatePrev{}; // 前フレームのコントローラ状態
	bool m_PadConnected = false; // コントローラ接続状態
	XINPUT_VIBRATION m_Vibration{}; // コントローラ振動状態

	bool m_KeyBind = false; // キーバインドモード

};
