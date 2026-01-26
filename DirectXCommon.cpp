#include "DirectXCommon.h"
#include <cassert>
#include <format>
#include <iostream>
#include <filesystem>
#include <thread>
//#include "Logger.h"
#include "StringUtility.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

#include "externals/DirectXTex-mar2023/DirectXTex/DirectXTex.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex-mar2023/DirectXTex/d3dx12.h"
using namespace Microsoft::WRL;
//using namespace StringUtility;
//using namespace Logger;

//=====================================================================
// DIrectXCommonの初期化
//=====================================================================

void DirectXCommon::Initialize(WinApp* winApp)
{

	//WindowsAPIの初期化
	assert(winApp);
	this->winApp = winApp;

	InitializeFixFPS();
	//デバイスの生成
	Deviceinitialize();
	//コマンド関連の初期化
	CommandListInitialize();
	//スワップチェーンの生成
	SwapChainInitialize();
	//ディスクリプタヒープの作成
	CreateDescriptorHeaps();
	//レンダーターゲットビューの作成
	CreateRTV();
	//深度ステンシルビューの作成
	CreateDSV();
	//フェンスの作成
	CreateFence();
	//ビューポートの設定
	SetViewportRect();
	//シザリング矩形の設定
	SetScissorRect();
	//DXCコンパイラの初期化
	CreateDXCCompiler();
	//ImGuiの初期化
	InitializeImGui();
}



void DirectXCommon::Deviceinitialize()
{
#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		// デバックレイヤーを有効化する
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif // _DEBUG

	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでassertにしとく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	// いい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference
	(
		i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i
		)
	{
		// アダプタ―の情報を習得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));// 取得できないのは一大事
		// ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			//Log(logStream, ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする

	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);





	// 昨日レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr))
		{
			// 生成できたのでログ出力を行ってループを抜ける
			//Log(logStream, std::format("FeatrueLevel : {}\n", featureLevelStrings[i]));
			break;
		}

	}
	// デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	//Log(logStream, ConvertString(L"Complete create D3D12Device!!!\n"));// 初期化完了のログを出す
}


//==============================================================
// コマンドキュー / コマンドリスト初期化
//==============================================================

void DirectXCommon::CommandListInitialize()
{

	HRESULT hr;

	// コマンドキューを生成する

	D3D12_COMMAND_QUEUE_DESC commandQuesDesc{};
	hr = device->CreateCommandQueue(&commandQuesDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成が上手くいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドアフロケータを生成
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケータの生成が上手くいかなかったので起動出来ない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成が上手くいかなかったので起動できない
	assert(SUCCEEDED(hr));
}

//==============================================================
// スワップチェーン生成
//==============================================================
void DirectXCommon::SwapChainInitialize(){
	HRESULT hr;

	// スワップチェーンを生成する
	//DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;   //画面の幅
	swapChainDesc.Height = WinApp::kClientHeight; //画面の高さ

	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色の形式
	swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタにうつしたら、中身を破棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

}


Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible)
{
	//==================================================
	// 指定された種類のディスクリプタヒープを生成する関数
	//==================================================

	// 作成されるディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;

	// ディスクリプタヒープの設定構造体
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;                 // ヒープの種類（RTV / SRV / DSV など）
	descriptorHeapDesc.NumDescriptors = numDescriptors; // 確保するディスクリプタ数
	// シェーダーから参照するかどうか
	descriptorHeapDesc.Flags =
		shaderVisible ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// ディスクリプタヒープ生成
	HRESULT hr = device->CreateDescriptorHeap(
		&descriptorHeapDesc,
		IID_PPV_ARGS(&descriptorHeap));

	// 生成失敗は致命的エラー
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}



void DirectXCommon::CreateDescriptorHeaps()
{
	//==================================================
	// 各種ディスクリプタサイズの取得
	//==================================================
	// ディスクリプタはヒープの種類ごとにサイズが異なる
	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//==================================================
	// 各種ディスクリプタヒープ生成
	//==================================================
	// SRV用（テクスチャなど・シェーダーから参照する）
	srvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// RTV用（スワップチェーンのバックバッファ用）
	rtvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// DSV用（深度バッファ用）
	dsvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DirectXCommon::CreateRTV()
{
	HRESULT hr;

	//==================================================
	// SwapChain からバックバッファのリソースを取得
	//==================================================
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	//==================================================
	// RTV（Render Target View）の設定
	//==================================================
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // SRGBとして描画結果を書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして扱う

	// RTVディスクリプタヒープの先頭ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
		rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVディスクリプタ1つ分のサイズ
	const UINT incrementSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//==================================================
	// バックバッファごとにRTVを作成
	//==================================================
	for (uint32_t i = 0; i < 2; i++)
	{
		// 現在のRTVハンドルを保存
		rtvHandles[i] = rtvStartHandle;

		// 各バックバッファに対応するRTVを生成
		device->CreateRenderTargetView(
			swapChainResources[i].Get(),
			&rtvDesc,
			rtvHandles[i]);

		// 次のRTVディスクリプタ位置へ進める
		rtvStartHandle.ptr += incrementSize;
	}
}

void DirectXCommon::CreateDSV()
{
	//==================================================
	// DSV用ディスクリプタヒープ生成
	//==================================================
	dsvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	//==================================================
	// 深度ステンシル用リソース生成
	//==================================================
	depthStencilResource =
		CreateDepthStencilTextureResource(
			WinApp::kClientWidth,
			WinApp::kClientHeight);

	//==================================================
	// DSV（Depth Stencil View）の設定
	//==================================================
	D3D12_DEPTH_STENCIL_VIEW_DESC devDesc{};
	devDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Resourceと同じフォーマット
	devDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ

	// DSV作成
	device->CreateDepthStencilView(
		depthStencilResource.Get(),
		&devDesc,
		dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFence()
{
	HRESULT hr;

	//==================================================
	// GPU と CPU の同期用 Fence 作成
	//==================================================
	fence = nullptr;
	uint64_t fenceValue = 0;

	hr = device->CreateFence(
		fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//==================================================
	// Fence待機用イベント作成
	//==================================================
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::SetViewportRect()
{
	//==================================================
	// ビューポート設定（描画領域）
	//==================================================
	viewport.Width = winApp->kClientWidth;
	viewport.Height = winApp->kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXCommon::SetScissorRect()
{
	//==================================================
	// シザー矩形設定（描画を行う範囲制限）
	// 基本的にビューポートと同じサイズにする
	//==================================================
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;
}

void DirectXCommon::CreateDXCCompiler()
{
	HRESULT hr;

	//==================================================
	// DXC（DirectX Shader Compiler）関連の初期化
	//==================================================

	// DXCユーティリティ作成（ファイル読み込み等を担当）
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));

	// DXCコンパイラ本体作成
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// #include 用のデフォルトハンドラ作成
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeImGui()
{
	//==================================================
	// ImGui 初期化処理
	//==================================================

	// ImGui のバージョンチェック
	IMGUI_CHECKVERSION();

	// ImGui コンテキスト生成
	ImGui::CreateContext();

	// ダークテーマ適用
	ImGui::StyleColorsDark();

	// Win32（ウィンドウ）との紐づけ
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// DirectX12 用 ImGui 初期化
	ImGui_ImplDX12_Init(
		device.Get(),
		swapChainDesc.BufferCount,      // バックバッファ数
		rtvDesc.Format,                 // RTVフォーマット
		srvDescriptorHeap.Get(),        // SRV用ディスクリプタヒープ
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index)
{
	//==================================================
	// CPU側ディスクリプタハンドル取得
	//==================================================

	// ヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
		descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// index分だけオフセットを進める
	handleCPU.ptr += (descriptorSize * index);

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index)
{
	//==================================================
	// GPU側ディスクリプタハンドル取得
	//==================================================

	// ヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
		descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// index分だけオフセットを進める
	handleGPU.ptr += (descriptorSize * index);

	return handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
	// SRV用CPUディスクリプタハンドル取得
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
	// SRV用GPUディスクリプタハンドル取得
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateDepthStencilTextureResource(int32_t width, int32_t height)
{
	//==================================================
	// 深度ステンシル用テクスチャリソース生成
	//==================================================

	// Resource設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                     // テクスチャ幅
	resourceDesc.Height = height;                   // テクスチャ高さ
	resourceDesc.MipLevels = 1;                     // mipmapなし
	resourceDesc.DepthOrArraySize = 1;              // 配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencil用フォーマット
	resourceDesc.SampleDesc.Count = 1;              // MSAAなし
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// VRAM上に作成するHeap設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度クリア時の初期値設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 最大深度で初期化
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Resource生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度書き込み状態
		&depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

void DirectXCommon::PreDraw()
{
	//==================================================
	// 描画前処理
	//==================================================

	// 現在描画対象のバックバッファ番号を取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// リソースバリア設定（Present → RenderTarget）
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// バリアを適用
	commandList->ResourceBarrier(1, &barrier);

	// 描画先RTV設定
	commandList->OMSetRenderTargets(
		1, &rtvHandles[backBufferIndex], false, nullptr);

	// 描画先DSV設定
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(
		1, &rtvHandles[backBufferIndex], false, &dsvHandle);

	// 画面クリア（色）
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList->ClearRenderTargetView(
		rtvHandles[backBufferIndex],
		clearColor,
		0,
		nullptr);

	// 深度バッファクリア
	commandList->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr);

	// 描画で使用するディスクリプタヒープ設定
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorheaps[] =
	{ srvDescriptorHeap };
	commandList->SetDescriptorHeaps(
		1, descriptorheaps->GetAddressOf());

	// ビューポート・シザー設定
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw()
{
	HRESULT hr;

	//==================================================
	// 描画後処理
	//==================================================

	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// RenderTarget → Present に戻す
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	// コマンドリスト確定
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUへコマンド実行
	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(
		1, commandLists->GetAddressOf());

	UpdateFixFPS();

	// 画面表示
	swapChain->Present(1, 0);

	// Fence更新
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);

	// GPU完了待ち
	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// 次フレーム用にリセット
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

Microsoft::WRL::ComPtr<IDxcBlob>
DirectXCommon::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	//==================================================
	// HLSL シェーダーのコンパイル処理
	//==================================================

	// HLSLファイル読み込み
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(
		filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// 読み込んだデータをDXC用バッファに設定
	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイルオプション
	LPCWSTR arguments[] =
	{
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"-Qembed_debug",
		L"-Od",
		L"-Zpr"
	};

	// コンパイル実行
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult));
	assert(SUCCEEDED(hr));

	// エラーチェック
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(
		DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError && shaderError->GetStringLength() != 0)
	{
		assert(false);
	}

	// コンパイル結果取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(
		DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
	//==================================================
	// UploadHeap 上にバッファリソースを作成
	//==================================================

	Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;

	// UploadHeap設定（CPUから書き込み可能）
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソース設定
	D3D12_RESOURCE_DESC vertexResourcceDesc{};
	vertexResourcceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourcceDesc.Width = sizeInBytes;
	vertexResourcceDesc.Height = 1;
	vertexResourcceDesc.DepthOrArraySize = 1;
	vertexResourcceDesc.MipLevels = 1;
	vertexResourcceDesc.SampleDesc.Count = 1;
	vertexResourcceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// Resource生成
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourcceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferResource));

	return bufferResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	//==================================================
	// テクスチャ用リソース生成
	//==================================================

	// Resource設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension =
		D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// VRAM上に作成
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resource生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}



[[nodiscard]] // 戻り値（中間リソース）を必ず受け取るべき関数
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::UploadTextureData(
	Microsoft::WRL::ComPtr<ID3D12Resource> texture,
	const DirectX::ScratchImage& mipImages)
{
	//==================================================
	// テクスチャデータをGPUリソースへアップロードする
	//==================================================

	// 各ミップレベルのサブリソース情報を格納する配列
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	// ScratchImage から Upload 用のサブリソース情報を作成
	DirectX::PrepareUpload(
		device.Get(),
		mipImages.GetImages(),
		mipImages.GetImageCount(),
		mipImages.GetMetadata(),
		subresources);

	// アップロードに必要な中間バッファサイズを計算
	uint64_t intermediateSize =
		GetRequiredIntermediateSize(
			texture.Get(),
			0,
			static_cast<UINT>(subresources.size()));

	// 中間バッファ（UploadHeap）を作成
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		CreateBufferResource(intermediateSize);

	// 中間バッファ経由でテクスチャへデータ転送
	UpdateSubresources(
		commandList.Get(),
		texture.Get(),
		intermediateResource.Get(),
		0,
		0,
		static_cast<UINT>(subresources.size()),
		subresources.data());

	//==================================================
	// リソース状態をコピー用 → シェーダー参照用に遷移
	//==================================================
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

	// バリア適用
	commandList->ResourceBarrier(1, &barrier);

	// 中間リソースはアップロード完了まで保持する必要があるため返す
	return intermediateResource;
}

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath)
{
	//==================================================
	// 画像ファイルを読み込み、ミップマップ付きデータを生成
	//==================================================

	// 画像データ格納用
	DirectX::ScratchImage image{};

	// std::string → std::wstring へ変換
	std::wstring filePathw = StringUtility::ConvertString(filePath);

	// WIC を使って画像ファイルを読み込む（SRGB強制）
	HRESULT hr = DirectX::LoadFromWICFile(
		filePathw.c_str(),
		DirectX::WIC_FLAGS_FORCE_SRGB,
		nullptr,
		image);
	assert(SUCCEEDED(hr));

	//==================================================
	// ミップマップ生成
	//==================================================
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(
		image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB,
		0,
		mipImages);

	// ミップマップ付きの画像データを返す
	return mipImages;
}

void DirectXCommon::InitializeFixFPS()
{
	//==================================================
	// FPS固定処理の基準時刻を初期化
	//==================================================
	reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFPS()
{
	//==================================================
	// 60FPS 固定処理
	//==================================================

	// 1フレームあたりの理想時間（1/60秒）
	const std::chrono::microseconds kMinTime(
		uint64_t(1000000.0f / 60.0f));

	// 少し短めのチェック用時間（余裕を持たせる）
	const std::chrono::microseconds kMinCheckTime(
		uint64_t(1000000.0f / 65.0f));

	// 現在時刻取得
	std::chrono::steady_clock::time_point now =
		std::chrono::steady_clock::now();

	// 前回フレームからの経過時間
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(
			now - reference_);

	// まだ1/60秒経過していない場合
	if (elapsed < kMinCheckTime)
	{
		// 1/60秒経過するまで細かくスリープ
		while (std::chrono::steady_clock::now() - reference_ < kMinTime)
		{
			// 1マイクロ秒スリープ
			std::this_thread::sleep_for(
				std::chrono::microseconds(1));
		}
	}

	// 次フレーム用に基準時刻更新
	reference_ = std::chrono::steady_clock::now();
}
