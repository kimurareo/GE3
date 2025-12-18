#include "DirectXCommon.h"
#include <cassert> 
#include <format>
#include <string>
#include "WinApp.h"
#include "Logger.h"
#include "StringUtility.h"

using namespace Logger;
using namespace StringUtility;

void DirectXCommon::Initialize(WinApp* winApp)
{

    winApp_ = winApp;

    CreateDevice();
    CreateCommand();
    CreateSwapChain();
    CreateRTV();
    CreateFence();

}

void DirectXCommon::CreateDevice() {

    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));

    for (UINT i = 0;
        dxgiFactory_->EnumAdapterByGpuPreference(
            i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter_)) != DXGI_ERROR_NOT_FOUND;
        ++i) {

        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = adapter_->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

           Log(ConvertString(
                std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
            break;
        }

        adapter_.Reset();
    }

    // 適切なアダプタが見つからなかったので起動できない
    assert(adapter_);


    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(
            adapter_.Get(),
            featureLevels[i],
            IID_PPV_ARGS(&device_));

        if (SUCCEEDED(hr)) {
            Log(std::format(
                "FeatureLevel : {}\n",
                featureLevelStrings[i]));
            break;
        }
    }

    // デバイスの生成が上手くいかなかったので起動できない
    assert(device_);
    Log("Complete create D3D12Device!!!\n");
}

void DirectXCommon::CreateCommand()
{
    HRESULT hr = S_OK;

    // コマンドキュー
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device_->CreateCommandQueue(
        &commandQueueDesc,
        IID_PPV_ARGS(&commandQueue_));
    assert(SUCCEEDED(hr));

    // コマンドアロケータ
    hr = device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator_));
    assert(SUCCEEDED(hr));

    // コマンドリスト
    hr = device_->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_));
    assert(SUCCEEDED(hr));

    // 最初は Close しておく（重要）
    hr = commandList_->Close();
    assert(SUCCEEDED(hr));

}

void DirectXCommon::CreateSwapChain()
{

    HRESULT hr = S_OK;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = WinApp::kClientWidth;
    swapChainDesc.Height = WinApp::kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> swapChain1;
    hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        winApp_->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1);
    assert(SUCCEEDED(hr));

    hr = swapChain1.As(&swapChain_);
    assert(SUCCEEDED(hr));
}


void DirectXCommon::CreateRTV()
{
    HRESULT hr;

    //==================================================
    // RTV用ディスクリプタヒープ作成
    //==================================================
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = kBackBufferNum;

    hr = device_->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(rtvDescriptorHeap_.GetAddressOf())
    );
    assert(SUCCEEDED(hr));

    // ディスクリプタ1個分のサイズを取得
    rtvDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    //==================================================
    // SwapChain からバックバッファを取得
    //==================================================
    for (uint32_t i = 0; i < kBackBufferNum; ++i) {
        hr = swapChain_->GetBuffer(
            i,
            IID_PPV_ARGS(backBuffers_[i].GetAddressOf())
        );
        assert(SUCCEEDED(hr));
    }

    //==================================================
    // RTV を作成
    //==================================================
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // ヒープの先頭ハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    for (uint32_t i = 0; i < kBackBufferNum; ++i) {
        rtvHandles_[i] = handle;

        device_->CreateRenderTargetView(
            backBuffers_[i].Get(),
            &rtvDesc,
            rtvHandles_[i]
        );

        // 次のディスクリプタ位置へ
        handle.ptr += rtvDescriptorSize_;
    }
}

