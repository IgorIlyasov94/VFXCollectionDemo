#include "DirectX12Renderer.h"
#include "DirectX12Helper.h"

Graphics::DirectX12Renderer::DirectX12Renderer(HWND windowHandler, uint32_t initialWidth, uint32_t initialHeight, bool fullscreen)
	: fenceEvent(nullptr), bufferIndex(0), fenceValues{}, resourceManager(nullptr), _windowHandler(windowHandler)
{
	initialWidth = std::max(initialWidth, 1u);
	initialHeight = std::max(initialHeight, 1u);

	sceneViewport.TopLeftX = 0.0f;
	sceneViewport.TopLeftY = 0.0f;
	sceneViewport.Width = static_cast<float>(initialWidth);
	sceneViewport.Height = static_cast<float>(initialHeight);
	sceneViewport.MinDepth = D3D12_MIN_DEPTH;
	sceneViewport.MaxDepth = D3D12_MAX_DEPTH;

	sceneScissorRect.left = 0;
	sceneScissorRect.top = 0;
	sceneScissorRect.right = initialWidth;
	sceneScissorRect.bottom = initialHeight;

	Initialize(initialWidth, initialHeight, fullscreen);
}

Graphics::DirectX12Renderer::~DirectX12Renderer()
{

}

void Graphics::DirectX12Renderer::GpuRelease()
{
	WaitForGpu();

	swapChain->SetFullscreenState(false, nullptr);

	CloseHandle(fenceEvent);
}

void Graphics::DirectX12Renderer::OnResize(uint32_t newWidth, uint32_t newHeight)
{
	ResetSwapChainBuffers(newWidth, newHeight);
}

void Graphics::DirectX12Renderer::OnSetFocus()
{
	WaitForGpu();

	swapChain->SetFullscreenState(isFullscreen, nullptr);

	for (auto& fenceValue : fenceValues)
		fenceValue = fenceValues[bufferIndex];

	const auto& bufferData = resourceManager->GetResourceData(swapChainBuffers[0]);

	ResetSwapChainBuffers(static_cast<uint32_t>(bufferData.textureInfo->width), static_cast<uint32_t>(bufferData.textureInfo->height));
}

void Graphics::DirectX12Renderer::OnLostFocus()
{
	WaitForGpu();

	swapChain->SetFullscreenState(false, nullptr);

	for (auto& fenceValue : fenceValues)
		fenceValue = fenceValues[bufferIndex];

	const auto& bufferData = resourceManager->GetResourceData(swapChainBuffers[0]);

	ResetSwapChainBuffers(static_cast<uint32_t>(bufferData.textureInfo->width), static_cast<uint32_t>(bufferData.textureInfo->height));
}

void Graphics::DirectX12Renderer::OnDeviceLost()
{
	const auto& bufferData = resourceManager->GetResourceData(swapChainBuffers[0]);
	auto width = bufferData.textureInfo->width;
	auto height = bufferData.textureInfo->height;

	ReleaseResources();
	Initialize(static_cast<uint32_t>(width), static_cast<uint32_t>(height), isFullscreen);
}

ID3D12GraphicsCommandList6* Graphics::DirectX12Renderer::FrameStart()
{
	ThrowIfFailed(commandAllocator->Reset(), "DirectX12Renderer::FrameStart: Command Allocator resetting error!");
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr), "DirectX12Renderer::FrameStart: Command List resetting error!");

	ID3D12DescriptorHeap* descHeaps[] = { resourceManager->GetCurrentCbvSrvUavDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	return commandList.Get();
}

void Graphics::DirectX12Renderer::FrameEnd()
{
	ThrowIfFailed(commandList->Close(), "DirectX12Renderer::FrameRender: Command List closing error!");

	ID3D12CommandList* commandLists[] = { commandList.Get() };

	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(swapChain->Present(1, 0), "DirectX12Renderer::FrameRender: Frame not presented!");

	PrepareNextFrame();
}

Memory::ResourceManager* Graphics::DirectX12Renderer::GetResourceManager()
{
	return resourceManager.get();
}

ID3D12Device2* Graphics::DirectX12Renderer::GetDevice()
{
	return device.Get();
}

void Graphics::DirectX12Renderer::SwitchFullscreenMode(bool enableFullscreen)
{
	if (isFullscreen == enableFullscreen)
		return;

	ThrowIfFailed(swapChain->SetFullscreenState(enableFullscreen, nullptr),
		"DirectX12Renderer::SwitchFullscreenMode: Fullscreen transition failed!");

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChain->GetDesc1(&swapChainDesc);

	ThrowIfFailed(swapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, swapChainDesc.Width,
		swapChainDesc.Height, swapChainDesc.Format, swapChainDesc.Flags),
		"DirectX12Renderer::SwitchFullscreenMode: Back buffers resizing error!");

	isFullscreen = enableFullscreen;
}

void Graphics::DirectX12Renderer::Initialize(uint32_t initialWidth, uint32_t initialHeight, bool fullscreen)
{
	DirectX12Helper::CreateFactory(&dxgiFactory);

	ComPtr<IDXGIAdapter4> adapter;
	DirectX12Helper::GetHardwareAdapter(dxgiFactory.Get(), &adapter);

	ComPtr<ID3D12Device> device0;
	DirectX12Helper::CreateDevice(adapter.Get(), &device0);
	ThrowIfFailed(device0.As(&device), "DirectX12Renderer::Initialize: Device conversion error!");

	DirectX12Helper::CreateCommandQueue(device.Get(), &commandQueue);

	ComPtr<IDXGISwapChain1> swapChain1;
	DirectX12Helper::CreateSwapChain(dxgiFactory.Get(), commandQueue.Get(), _windowHandler, SWAP_CHAIN_BUFFER_COUNT,
		initialWidth, initialHeight, &swapChain1);

	ThrowIfFailed(swapChain1.As(&swapChain), "DirectX12Renderer::Initialize: Swap Chain conversion error!");

	isFullscreen = fullscreen;

	if (fullscreen)
		SwitchFullscreenMode(true);

	bufferIndex = swapChain->GetCurrentBackBufferIndex();

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
		"DirectX12Renderer::Initialize: Command Allocator creating error!");

	ThrowIfFailed(device->CreateFence(fenceValues[bufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
		"DirectX12Renderer::Initialize: Fence creating error!");
	fenceValues[bufferIndex]++;

	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (fenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "DirectX12Renderer::Initialize: Fence Event creating error!");

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)),
		"DirectX12Renderer::Initialize: Command List creating error!");

	ThrowIfFailed(commandList->Close(), "DirectX12Renderer::Initialize: Command List closing error!");

	if (resourceManager == nullptr)
		resourceManager = std::make_shared<Memory::ResourceManager>(device.Get());

	CreateSwapChainBuffers(initialWidth, initialHeight);

	//resourceManager->UpdateResources();

	//WaitForGpu();
}

void Graphics::DirectX12Renderer::CreateSwapChainBuffers(uint32_t initialWidth, uint32_t initialHeight)
{
	for (uint32_t bufferId = 0; bufferId < SWAP_CHAIN_BUFFER_COUNT; bufferId++)
	{
		Memory::ResourceDesc desc{};
		desc.textureInfo = std::make_shared<Graphics::TextureInfo>(Graphics::TextureInfo{});
		desc.textureInfo->width = initialWidth;
		desc.textureInfo->height = initialHeight;
		desc.textureInfo->depth = 1;
		desc.textureInfo->mipLevels = 1;
		desc.textureInfo->dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.textureInfo->format = SWAP_CHAIN_FORMAT;

		swapChain->GetBuffer(bufferId, IID_PPV_ARGS(&swapChainResources[bufferId]));

		desc.preparedResource = swapChainResources[bufferId].Get();

		swapChainBuffers[bufferId] = resourceManager->CreateResource(Memory::ResourceType::SWAP_CHAIN, desc);
		swapChainCPUDescriptors[bufferId] = resourceManager->GetRenderTargetDescriptorBase(swapChainBuffers[bufferId]);
	}
}

void Graphics::DirectX12Renderer::ReleaseResources()
{
	for (uint32_t bufferId = 0; bufferId < SWAP_CHAIN_BUFFER_COUNT; bufferId++)
	{
		swapChainResources[bufferId].Reset();
		resourceManager->ReleaseResource(swapChainBuffers[bufferId]);
	}

	commandAllocator.Reset();
	commandQueue.Reset();
	commandList.Reset();
	fence.Reset();
	swapChain.Reset();
	device.Reset();
	dxgiFactory.Reset();
}

void Graphics::DirectX12Renderer::ResetSwapChainBuffers(uint32_t width, uint32_t height)
{
	WaitForGpu();

	for (auto& swapChainResource : swapChainResources)
		swapChainResource.Reset();

	width = std::max(width, 1u);
	height = std::max(height, 1u);

	auto hResult = swapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, SWAP_CHAIN_FORMAT, 0);

	if (hResult == DXGI_ERROR_DEVICE_REMOVED || hResult == DXGI_ERROR_DEVICE_RESET)
	{
		OnDeviceLost();

		return;
	}
	else
		ThrowIfFailed(hResult, "DirectX12Renderer::ResetSwapChainBuffers: Resizing Swap Chain error!");

	for (uint32_t bufferId = 0; bufferId < SWAP_CHAIN_BUFFER_COUNT; bufferId++)
	{
		swapChain->GetBuffer(bufferId, IID_PPV_ARGS(&swapChainResources[bufferId]));

		D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
		renderTargetViewDesc.Format = SWAP_CHAIN_FORMAT;
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		device->CreateRenderTargetView(swapChainResources[bufferId].Get(), &renderTargetViewDesc, swapChainCPUDescriptors[bufferId]);
	}

	bufferIndex = swapChain->GetCurrentBackBufferIndex();

	sceneViewport.TopLeftX = 0.0f;
	sceneViewport.TopLeftY = 0.0f;
	sceneViewport.Width = static_cast<float>(width);
	sceneViewport.Height = static_cast<float>(height);
	sceneViewport.MinDepth = D3D12_MIN_DEPTH;
	sceneViewport.MaxDepth = D3D12_MAX_DEPTH;

	sceneScissorRect.left = 0;
	sceneScissorRect.top = 0;
	sceneScissorRect.right = width;
	sceneScissorRect.bottom = height;
}

void Graphics::DirectX12Renderer::PrepareNextFrame()
{
	auto currentFenceValue = fenceValues[bufferIndex];
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue), "DirectX12Renderer::PrepareNextFrame: Signal error!");

	bufferIndex = swapChain->GetCurrentBackBufferIndex();

	if (fence->GetCompletedValue() < fenceValues[bufferIndex])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[bufferIndex], fenceEvent), "DirectX12Renderer::PrepareNextFrame: Fence error!");
		WaitForSingleObjectEx(fenceEvent, INFINITE, false);
	}

	fenceValues[bufferIndex] = currentFenceValue + 1;
}

void Graphics::DirectX12Renderer::WaitForGpu()
{
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValues[bufferIndex]), "DirectX12Renderer::WaitForGpu: Signal error!");
	ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[bufferIndex], fenceEvent), "DirectX12Renderer::WaitForGpu: Fence error!");

	WaitForSingleObjectEx(fenceEvent, INFINITE, false);

	fenceValues[bufferIndex]++;
}
