#pragma once

#include "DirectX12Includes.h"
#include "CommandManager.h"
#include "DescriptorManager.h"
#include "BufferManager.h"
#include "TextureManager.h"
#include "Resources/ResourceManager.h"

namespace Graphics
{
	class DirectX12Renderer final
	{
	public:
		DirectX12Renderer(const RECT& windowPlacement, HWND windowHandler, bool _isFullscreen);
		~DirectX12Renderer();

		bool ToggleFullscreen();
		void SetFullscreen(bool _isFullscreen);

		void OnResize(uint32_t newWidth, uint32_t newHeight, HWND windowHandler);
		void OnSetFocus(HWND windowHandler);
		void OnLostFocus(HWND windowHandler);
		void OnDeviceLost(HWND windowHandler);

		ID3D12GraphicsCommandList* StartFrame();
		void EndFrame(ID3D12GraphicsCommandList* commandList);

		void ResetDescriptorHeaps(ID3D12GraphicsCommandList* commandList);

		void SetRenderToBackBuffer(ID3D12GraphicsCommandList* commandList);

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		const uint2& GetDisplaySize() const;

		ID3D12Device* GetDevice();

		CommandManager* GetCommandManager();
		Resources::ResourceManager* GetResourceManager();
		BufferManager* GetBufferManager();

		ID3D12GraphicsCommandList* StartCreatingResources();
		void EndCreatingResources();

		void FlushQueue();
		void FlushResourcesQueue();

		static const uint32_t BACK_BUFFER_NUMBER = 2u;
		static const DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;
		static constexpr float BACK_BUFFER_COLOR[4] = { 1.0f, 0.5f, 0.75f, 1.0f };

	private:
		DirectX12Renderer() = delete;
		DirectX12Renderer(const DirectX12Renderer&) = delete;
		DirectX12Renderer(DirectX12Renderer&&) = delete;
		DirectX12Renderer& operator=(const DirectX12Renderer&) = delete;
		DirectX12Renderer& operator=(DirectX12Renderer&&) = delete;

		void CreateGPUResources(uint32_t width, uint32_t height, HWND windowHandler);

		ID3D12Device2* CreateDevice(IDXGIFactory4* factory);
		IDXGIFactory4* CreateFactory();
		IDXGIAdapter1* FindHighestPerformanceAdapter(IDXGIFactory4* factory);
		IDXGISwapChain3* CreateSwapChain(uint32_t width, uint32_t height, HWND windowHandler,
			IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue);

		void ResetSwapChain(uint32_t width, uint32_t height, HWND windowHandler);

		void WaitForGPU(CommandQueueID _commandQueueId);
		void PrepareToNextFrame();

		ID3D12Device2* device;
		ID3D12Fence* fence;

		std::array<ID3D12Resource*, BACK_BUFFER_NUMBER> backBuffers;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, BACK_BUFFER_NUMBER> backBufferCPUDescriptors;
		
		IDXGISwapChain3* swapChain;

		HANDLE fenceEvent;
		std::array<uint64_t, BACK_BUFFER_NUMBER> fenceValues;

		CommandListID commandListId;
		std::array<CommandAllocatorID, BACK_BUFFER_NUMBER> commandAllocators;
		CommandQueueID commandQueueId;

		CommandListID resourceCommandListId;
		std::array<CommandAllocatorID, BACK_BUFFER_NUMBER> resourceCommandAllocators;
		CommandQueueID resourceCommandQueueId;

		CommandManager* commandManager;
		DescriptorManager* descriptorManager;
		BufferManager* bufferManager;
		TextureManager* textureManager;
		Resources::ResourceManager* resourceManager;

		uint32_t currentWidth;
		uint32_t currentHeight;
		uint32_t bufferIndex;

		uint2 displaySize;

		RECT lastWindowRect;

		bool isFullscreen;
	};
}
