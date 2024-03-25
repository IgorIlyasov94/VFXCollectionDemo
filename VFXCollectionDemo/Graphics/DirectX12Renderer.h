#pragma once

#include "../Includes.h"
#include "Resources/ResourceManager.h"

namespace Graphics
{
	class DirectX12Renderer final
	{
	public:
		DirectX12Renderer(HWND windowHandler, uint32_t initialWidth, uint32_t initialHeight, bool fullscreen);
		~DirectX12Renderer();

		void GpuRelease();

		void OnResize(uint32_t newWidth, uint32_t newHeight);
		void OnSetFocus();
		void OnLostFocus();
		void OnDeviceLost();
		
		ID3D12GraphicsCommandList6* FrameStart();
		void FrameEnd();

		Memory::ResourceManager* GetResourceManager();

		ID3D12Device2* GetDevice();

		void SwitchFullscreenMode(bool enableFullscreen);

	private:
		DirectX12Renderer() = delete;
		DirectX12Renderer(const DirectX12Renderer&) = delete;
		DirectX12Renderer(DirectX12Renderer&&) = delete;
		DirectX12Renderer& operator=(const DirectX12Renderer&) = delete;
		DirectX12Renderer& operator=(DirectX12Renderer&&) = delete;

		void Initialize(uint32_t initialWidth, uint32_t initialHeight, bool fullscreen);
		void CreateSwapChainBuffers(uint32_t initialWidth, uint32_t initialHeight);
		void ReleaseResources();

		void ResetSwapChainBuffers(uint32_t width, uint32_t height);
		void PrepareNextFrame();
		void WaitForGpu();
		
		static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 2;
		static const DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

		ComPtr<ID3D12Device2> device;
		ComPtr<ID3D12GraphicsCommandList6> commandList;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12Fence1> fence;

		ComPtr<IDXGIFactory7> dxgiFactory;
		ComPtr<IDXGISwapChain4> swapChain;

		std::shared_ptr<Memory::ResourceManager> resourceManager;
		Memory::ResourceId swapChainBuffers[SWAP_CHAIN_BUFFER_COUNT];
		ComPtr<ID3D12Resource> swapChainResources[SWAP_CHAIN_BUFFER_COUNT];
		D3D12_CPU_DESCRIPTOR_HANDLE swapChainCPUDescriptors[SWAP_CHAIN_BUFFER_COUNT];

		HANDLE fenceEvent;

		D3D12_VIEWPORT sceneViewport;
		D3D12_RECT sceneScissorRect;

		HWND _windowHandler;

		bool isFullscreen;

		uint32_t bufferIndex;
		uint64_t fenceValues[SWAP_CHAIN_BUFFER_COUNT];
	};
}
