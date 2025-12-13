#include "Input.h"
#include <cassert>
#include "WinApp.h"


#pragma comment (lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")


void Input::Initialize(WinApp* winApp)
{

	HRESULT result;

	// 借りてきたwinAppのインスタンスを記録
	this->winApp = winApp;

	// DirectInputの初期化
	//IDirectInput8* directInput = nullptr;
    result = DirectInput8Create(winApp->GetInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	//IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

}

void Input::Updata()
{

	// 前回のキー入力を保存
	memcpy(preKey, key, sizeof(key));

	// キーボードの情報の取得開始
	keyboard->Acquire();

	keyboard->GetDeviceState(sizeof(key), key);

	if (key[ DIK_0 ]) {
	 OutputDebugStringA("Hit 0\n");
	}

}

bool Input::PushKey(BYTE keyNumber)
{

	// 指定キーを押していればtureを返す
	if (key[keyNumber]) {
		return true;
	}

	// そうでなければfalseを返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	if (key[keyNumber] && !preKey[keyNumber]) {
		return true;
	}

	return false;
}
