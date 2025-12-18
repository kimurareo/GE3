#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>


using Microsoft::WRL::ComPtr;



class WinApp;

class DirectXCommon {
public:
    static const uint32_t kBackBufferNum = 2;

    void Initialize(WinApp* winApp);
    
private:

    WinApp* winApp_ = nullptr;
    // 初期化分割
    void CreateDevice();
    void CreateCommand();
    void CreateSwapChain();
    void CreateRTV();
    void CreateFence();

private:
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<IDXGISwapChain4> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    ComPtr<ID3D12Resource> backBuffers_[kBackBufferNum];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferNum]{};
    uint32_t rtvDescriptorSize_ = 0;


};
