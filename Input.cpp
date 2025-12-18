#include "Input.h"
#include<cassert>
#include<wrl.h>
using namespace Microsoft::WRL;
#define DIRECTINPUT_VERSION 0x0800// DirectInput8のバージョン指定
//#include <dinput.h>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")


void Input::Initialize(WinApp* winApp)
{
	HRESULT hr;

	//DirectInputの初期化
	//ComPtr<IDirectInput8> directInput = nullptr;
	hr = DirectInput8Create
	(
		winApp->GetHInstance(), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (void**)&directInput,
		nullptr
	);
	//assert(SUCCEEDED(hr));


	//キーボードデバイスの生成
	//ComPtr<IDirectInputDevice8> keyboard = nullptr;
	hr = directInput->CreateDevice
	(
		GUID_SysKeyboard, &keyboard,
		nullptr
	);
	//assert(SUCCEEDED(hr));


	//入力データ形式のセット
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);//標準形式
	//assert(SUCCEEDED(hr));


	//排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel
	(
		winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);
	//assert(SUCCEEDED(hr));




	//借りてきたWinAppのインスタンスを記録
	this->winApp = winApp;


}

void Input::Update()
{


	//前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));


	//キーボード入力情報取得開始
	keyboard->Acquire();
	//全キーの入力情報を取得する
	//BYTE key[256] = {};
	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::TriggerKey(BYTE keyNumber)
{
	// 今回は押している && 前回は押していない
	if (key[keyNumber] && !keyPre[keyNumber])
	{
		return true;
	} else
	{
		return false;
	}
	return false;
}

bool Input::PushKey(BYTE keyNumber)
{
	//指定キーを押していればtrueを返す
	if (key[keyNumber])
	{
		return true;
	}
	//そうでなければfalseを返す
	return false;
}