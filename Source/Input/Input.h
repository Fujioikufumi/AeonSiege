#ifndef __INPUT_H__
#define __INPUT_H__

#include <Windows.h>
#undef max
#undef min

//-----------------------------------------------------------------------------
// マウス状態（スクリーン座標・ボタン）
//-----------------------------------------------------------------------------
struct MouseState
{
	int x, y;
	int deltaX, deltaY;
	bool leftButton;
	bool rightButton;
	bool middleButton;
};

//-----------------------------------------------------------------------------
// XInput: Xbox 互換コントローラのボタンは wButtons のビットフラグ
// Microsoft 定義の XINPUT_GAMEPAD_* と同じ値（引数で WORD を渡す）
//-----------------------------------------------------------------------------
#define PAD_UP               0x0001
#define PAD_DOWN             0x0002
#define PAD_LEFT             0x0004
#define PAD_RIGHT            0x0008
#define PAD_START            0x0010
#define PAD_BACK             0x0020
#define PAD_LEFT_THUMB       0x0040
#define PAD_RIGHT_THUMB      0x0080
#define PAD_LEFT_SHOULDER    0x0100
#define PAD_RIGHT_SHOULDER   0x0200
#define PAD_A                0x1000
#define PAD_B                0x2000
#define PAD_X                0x4000
#define PAD_Y                0x8000

// スティック値の正規化用（sThumbL/R は SHORT 相当 -32768?32767）
#define STICK_FORCE_MAX (32767.0f)
#define STICK_FORCE_MIN (-32768.0f)

HRESULT InitInput();
void UninitInput();
void UpdateInput();

bool IsKeyPress(BYTE key);
bool IsKeyTrigger(BYTE key);
bool IsKeyRelease(BYTE key);
bool IsKeyRepeat(BYTE key);

void SetInputWindow(HWND hWnd);
void LockMouse(bool lock);
MouseState GetMouseState();
bool IsMouseButtonPress(int button);
bool IsMouseButtonTrigger(int button);
bool IsMouseButtonRelease(int button);

bool IsControllerPress(WORD button);
bool IsControllerTrigger(WORD button);
bool IsControllerRelease(WORD button);
bool IsControllerRepeat(WORD button);

bool IsLTPress(BYTE strength);
bool IsRTPress(BYTE strength);
bool IsLTTrigger();
bool IsRTTrigger();
bool IsLTRelease();
bool IsRTRelease();

bool IsLLeftStickPress();
bool IsLRightStickPress();
float IsLLeftStickForce();
float IsLRightStickForce();

bool IsRLeftStickPress();
bool IsRRightStickPress();
float IsRLeftStickForce();
float IsRRightStickForce();

float GetLeftStickX();
float GetLeftStickY();
float GetRightStickX();
float GetRightStickY();

bool IsLUpStickPress();
bool IsLDownStickPress();

void Vibration(int controllerNum, int leftMotorSpeed, int rightMotorSpeed);
void StopVibration(int controllerNum);

void KeyBind(bool inBind);
bool GetKeyBind();

#endif // __INPUT_H__

//#ifndef __INPUT_H__
//#define __INPUT_H__
//
//#include <Windows.h>
//#undef max
//#undef min
//
//// マウス構造体
//struct MouseState
//{
//    int x, y;           // 現在のマウス座標
//    int deltaX, deltaY; // 前フレームからの移動量
//    bool leftButton;    // 左ボタンの状態
//    bool rightButton;   // 右ボタンの状態
//    bool middleButton;  // 中ボタンの状態
//};
//
//HRESULT InitInput();
//void UninitInput();
//void UpdateInput();
//
//bool IsKeyPress(BYTE key);
//bool IsKeyTrigger(BYTE key);
//bool IsKeyRelease(BYTE key);
//bool IsKeyRepeat(BYTE key);
//
//// マウス入力関数
//MouseState GetMouseState();
//void SetInputWindow(HWND hWnd);
//void LockMouse(bool lock);
//MouseState GetMouseState();
//bool IsMouseButtonPress(int button);
//bool IsMouseButtonTrigger(int button);
//bool IsMouseButtonRelease(int button);
//
//#endif // __INPUT_H__