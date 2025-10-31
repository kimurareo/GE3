#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class Input {
public:
	// namespace省略
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:

	void Initialize(HINSTANCE hinstance,HWND hwnd);

	void Updata();

	bool PushKey(BYTE keyNumber);

	bool TriggerKey(BYTE keyNumber);

private:
	// キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard;

	// DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput;

	// 全キーの状態
	BYTE key[256] = {};
	BYTE preKey[256] = {};

};
