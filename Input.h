#pragma once
#include<Windows.h>
#include<wrl.h>
#define DIRECTINPUT_VERSION 0x0800// DirectInput8のバージョン指定
#include <dinput.h>
#include"WinApp.h"

//入力
class Input
{
public://メンバ関数
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;



	//初期化
	void Initialize(WinApp* winApp);
	//更新
	void Update();



	//全キー状態
	BYTE key[256] = {};
	//前回の全キー状態
	BYTE keyPre[256] = {};

	bool TriggerKey(BYTE keyNumber);

	bool PushKey(BYTE keyNumber);


private:
	//キーボードのデバイス
	ComPtr<IDirectInputDevice8>keyboard;

	ComPtr<IDirectInput8> directInput;
	//WindowsAPI
	WinApp* winApp = nullptr;


	WinApp* winApp_ = nullptr;
};