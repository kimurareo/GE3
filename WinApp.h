#pragma once
#include"Windows.h"
#include"cstdint"

//WindowsAPI
class WinApp
{
public:

	void Initialize();
	void Update();
	void Finalize();

	static LRESULT CALLBACK WindowProc
	(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;



	HINSTANCE GetHInstance()const { return wc.hInstance; }
	HWND GetHwnd()const { return hwnd; }


	//メッセージの処理
	bool ProceccMassage();

private:
	//ウインドウハンドル
	HWND hwnd = nullptr;

	//ウインドウクラスの設定
	WNDCLASS wc{};
};
