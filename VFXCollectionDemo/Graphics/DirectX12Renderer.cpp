#include "DirectX12Renderer.h"
#include "DirectX12Utilities.h"
#include "../Common/Window.h"

Graphics::DirectX12Renderer::DirectX12Renderer(const RECT& windowPlacement, HWND windowHandler, bool _isFullscreen)
    : commandManager(nullptr), descriptorManager(nullptr), commandQueueId{},
    isFullscreen(_isFullscreen), lastWindowRect{}, displaySize{}
{
    currentWidth = windowPlacement.right - windowPlacement.left;
    currentHeight = windowPlacement.bottom - windowPlacement.top;

    backBuffers.fill(nullptr);
    fenceValues.fill(0u);

    CreateGPUResources(currentWidth, currentHeight, windowHandler);
}

Graphics::DirectX12Renderer::~DirectX12Renderer()
{
#ifdef _DEBUG
    ID3D12Debug* _debug{};
    D3D12GetDebugInterface(IID_PPV_ARGS(&_debug));
    _debug->EnableDebugLayer();

    ID3D12DebugDevice* _debugDevice{};
    device->QueryInterface(IID_PPV_ARGS(&_debugDevice));
    _debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
#endif

    WaitForGPU(commandQueueId);

#ifdef _DEBUG
    _debugDevice->Release();
    _debug->Release();
#endif

    for (auto& buffer : backBuffers)
        buffer->Release();

    delete descriptorManager;
    delete resourceManager;
    delete textureManager;
    delete bufferManager;

    WaitForGPU(commandQueueId);

    delete commandManager;

    fence->Release();
    swapChain->Release();

    CloseHandle(fenceEvent);

    device->Release();
}

bool Graphics::DirectX12Renderer::ToggleFullscreen()
{
    isFullscreen = !isFullscreen;

    return isFullscreen;
}

void Graphics::DirectX12Renderer::SetFullscreen(bool _isFullscreen)
{
    if (isFullscreen != _isFullscreen)
        ToggleFullscreen();
}

void Graphics::DirectX12Renderer::OnResize(uint32_t newWidth, uint32_t newHeight, HWND windowHandler)
{
    currentWidth = newWidth;
    currentHeight = newHeight;

    ResetSwapChain(currentWidth, currentHeight, windowHandler);
}

void Graphics::DirectX12Renderer::OnSetFocus(HWND windowHandler)
{
    WaitForGPU(commandQueueId);

    swapChain->SetFullscreenState(isFullscreen, nullptr);

    for (auto& fenceValue : fenceValues)
        fenceValue = fenceValues[bufferIndex];

    ResetSwapChain(currentWidth, currentHeight, windowHandler);
}

void Graphics::DirectX12Renderer::OnLostFocus(HWND windowHandler)
{
    WaitForGPU(commandQueueId);

    swapChain->SetFullscreenState(false, nullptr);

    for (auto& fenceValue : fenceValues)
        fenceValue = fenceValues[bufferIndex];

    ResetSwapChain(currentWidth, currentHeight, windowHandler);
}

void Graphics::DirectX12Renderer::OnDeviceLost(HWND windowHandler)
{
    WaitForGPU(commandQueueId);
    
    for (auto& buffer : backBuffers)
        buffer->Release();

    fence->Release();
    swapChain->Release();
    device->Release();

    CloseHandle(fenceEvent);

    CreateGPUResources(currentWidth, currentHeight, windowHandler);
}

ID3D12GraphicsCommandList* Graphics::DirectX12Renderer::StartFrame()
{
    auto d3dCommandList = commandManager->BeginRecord(commandListId, commandAllocators[bufferIndex]);

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        descriptorManager->GetHeap(DescriptorType::CBV_SRV_UAV),
        descriptorManager->GetHeap(DescriptorType::SAMPLER)
    };
    d3dCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    return d3dCommandList;
}

void Graphics::DirectX12Renderer::EndFrame(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers[bufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    commandList->ResourceBarrier(1u, &barrier);

    commandManager->EndRecord(commandListId);
    commandManager->SubmitCommandList(commandListId);
    commandManager->ExecuteCommands(commandQueueId);

    //swapChain->Present(0u, DXGI_PRESENT_ALLOW_TEARING);
    swapChain->Present(1u, 0u);

    PrepareToNextFrame();
}

void Graphics::DirectX12Renderer::ResetDescriptorHeaps(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        descriptorManager->GetHeap(DescriptorType::CBV_SRV_UAV),
        descriptorManager->GetHeap(DescriptorType::SAMPLER)
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void Graphics::DirectX12Renderer::SetRenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers[bufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    commandList->ResourceBarrier(1u, &barrier);

    commandList->OMSetRenderTargets(1u, &backBufferCPUDescriptors[bufferIndex], true, nullptr);
}

uint32_t Graphics::DirectX12Renderer::GetWidth() const
{
    return currentWidth;
}

uint32_t Graphics::DirectX12Renderer::GetHeight() const
{
    return currentHeight;
}

const uint2& Graphics::DirectX12Renderer::GetDisplaySize() const
{
    return displaySize;
}

ID3D12Device* Graphics::DirectX12Renderer::GetDevice()
{
    return device;
}

Graphics::CommandManager* Graphics::DirectX12Renderer::GetCommandManager()
{
    return commandManager;
}

Graphics::Resources::ResourceManager* Graphics::DirectX12Renderer::GetResourceManager()
{
    return resourceManager;
}

Graphics::BufferManager* Graphics::DirectX12Renderer::GetBufferManager()
{
    return bufferManager;
}

ID3D12GraphicsCommandList* Graphics::DirectX12Renderer::StartCreatingResources()
{
    return commandManager->BeginRecord(resourceCommandListId, resourceCommandAllocators[bufferIndex]);
}

void Graphics::DirectX12Renderer::EndCreatingResources()
{
    commandManager->EndRecord(resourceCommandListId);
    commandManager->SubmitCommandList(resourceCommandListId);
    commandManager->ExecuteCommands(resourceCommandQueueId);

    WaitForGPU(resourceCommandQueueId);

    bufferManager->ReleaseTempBuffers();
    textureManager->ReleaseTempBuffers();
}

void Graphics::DirectX12Renderer::FlushQueue()
{
    WaitForGPU(commandQueueId);
}

void Graphics::DirectX12Renderer::FlushResourcesQueue()
{
    WaitForGPU(resourceCommandQueueId);
}

void Graphics::DirectX12Renderer::CreateGPUResources(uint32_t width, uint32_t height, HWND windowHandler)
{
    auto factory = CreateFactory();
    device = CreateDevice(factory);

    for (uint32_t backBufferId = 0u; backBufferId < BACK_BUFFER_NUMBER; backBufferId++)
        if (backBuffers[backBufferId] != nullptr)
            backBuffers[backBufferId]->Release();

    if (commandManager == nullptr)
    {
        commandManager = new CommandManager(device);
        commandQueueId = commandManager->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        resourceCommandQueueId = commandManager->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

        auto commandQueue = commandManager->GetQueue(commandQueueId);

        swapChain = CreateSwapChain(width, height, windowHandler, factory, commandQueue);
        bufferIndex = swapChain->GetCurrentBackBufferIndex();

        for (uint32_t backBufferId = 0u; backBufferId < BACK_BUFFER_NUMBER; backBufferId++)
        {
            commandAllocators[backBufferId] = commandManager->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
            resourceCommandAllocators[backBufferId] = commandManager->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
        }

        commandListId = commandManager->CreateCommandList(commandAllocators[bufferIndex]);
        resourceCommandListId = commandManager->CreateCommandList(resourceCommandAllocators[bufferIndex]);
        
        commandManager->SubmitCommandList(commandListId);
        commandManager->SubmitCommandList(resourceCommandListId);
        commandManager->ExecuteCommands(commandQueueId);
    }
    else
    {
        auto commandQueue = commandManager->GetQueue(commandQueueId);

        swapChain->Release();

        swapChain = CreateSwapChain(width, height, windowHandler, factory, commandQueue);
        bufferIndex = swapChain->GetCurrentBackBufferIndex();
    }

    if (descriptorManager == nullptr)
    {
        descriptorManager = new DescriptorManager(device);

        for (uint32_t backBufferId = 0u; backBufferId < BACK_BUFFER_NUMBER; backBufferId++)
        {
            auto allocation = descriptorManager->Allocate(DescriptorType::RTV);
            backBufferCPUDescriptors[backBufferId] = allocation.cpuDescriptor;
        }
    }

    if (bufferManager == nullptr)
        bufferManager = new BufferManager();

    if (textureManager == nullptr)
        textureManager = new TextureManager();

    if (resourceManager == nullptr)
        resourceManager = new Resources::ResourceManager(descriptorManager, bufferManager, textureManager);

    device->CreateFence(fenceValues[bufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    fenceEvent = CreateEvent(nullptr, false, false, nullptr);

    for (uint32_t backBufferId = 0u; backBufferId < BACK_BUFFER_NUMBER; backBufferId++)
    {
       swapChain->GetBuffer(backBufferId, IID_PPV_ARGS(&backBuffers[backBufferId]));

       D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
       rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
       rtvDesc.Format = BACK_BUFFER_FORMAT;
       
       device->CreateRenderTargetView(backBuffers[backBufferId], &rtvDesc, backBufferCPUDescriptors[backBufferId]);
    }

    WaitForGPU(commandQueueId);
    WaitForGPU(resourceCommandQueueId);

    factory->Release();
}

ID3D12Device2* Graphics::DirectX12Renderer::CreateDevice(IDXGIFactory4* factory)
{
#if defined(_DEBUG)
    ID3D12Debug* debugController = nullptr;
    //ID3D12Debug1* debugController1 = nullptr;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        //if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
        //{
        //    debugController1->SetEnableGPUBasedValidation(true);
        //    debugController1->Release();
        //}

        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    ID3D12Device2* newDevice = nullptr;

    auto adapter = FindHighestPerformanceAdapter(factory);
    D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&newDevice));
    newDevice->SetName(L"DEVICE");

    adapter->Release();

	return newDevice;
}

IDXGIFactory4* Graphics::DirectX12Renderer::CreateFactory()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    IDXGIFactory4* factory = nullptr;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

    return factory;
}

IDXGIAdapter1* Graphics::DirectX12Renderer::FindHighestPerformanceAdapter(IDXGIFactory4* factory)
{
    IDXGIAdapter1* adapter = nullptr;
    IDXGIFactory6* factory6;

    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (uint32_t adapterIndex = 0u;
            factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) > 0u)
                continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
            {
                factory6->Release();

                break;
            }
        }
    }

    if (adapter == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

	return adapter;
}

IDXGISwapChain3* Graphics::DirectX12Renderer::CreateSwapChain(uint32_t width, uint32_t height, HWND windowHandler,
    IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue)
{
    IDXGISwapChain1* newSwapChain1 = nullptr;
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = BACK_BUFFER_FORMAT;
    desc.SampleDesc.Count = 1;
    //desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = BACK_BUFFER_NUMBER;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc{};
    fullScreenDesc.Windowed = !isFullscreen;

    factory->CreateSwapChainForHwnd(commandQueue, windowHandler, &desc, &fullScreenDesc, nullptr, &newSwapChain1);
    factory->MakeWindowAssociation(windowHandler, DXGI_MWA_NO_ALT_ENTER);

    IDXGIOutput* output;
    newSwapChain1->GetContainingOutput(&output);

    DXGI_OUTPUT_DESC outputDesc{};
    output->GetDesc(&outputDesc);
    output->Release();

    displaySize.x = outputDesc.DesktopCoordinates.right;
    displaySize.y = outputDesc.DesktopCoordinates.bottom;

    IDXGISwapChain3* newSwapChain = nullptr;
    newSwapChain1->QueryInterface(IID_PPV_ARGS(&newSwapChain));

    newSwapChain1->Release();

    return newSwapChain;
}

void Graphics::DirectX12Renderer::ResetSwapChain(uint32_t width, uint32_t height, HWND windowHandler)
{
    WaitForGPU(commandQueueId);

    for (uint32_t bufferId = 0; bufferId < BACK_BUFFER_NUMBER; bufferId++)
    {
        backBuffers[bufferId]->Release();
        fenceValues[bufferId] = fenceValues[bufferIndex];
    }

    auto hResult = swapChain->ResizeBuffers(BACK_BUFFER_NUMBER, width, height, BACK_BUFFER_FORMAT, 0u);

    if (hResult == DXGI_ERROR_DEVICE_REMOVED || hResult == DXGI_ERROR_DEVICE_RESET)
    {
        hResult = device->GetDeviceRemovedReason();

        OnDeviceLost(windowHandler);

        return;
    }

    for (uint32_t bufferId = 0; bufferId < BACK_BUFFER_NUMBER; bufferId++)
    {
        swapChain->GetBuffer(bufferId, IID_PPV_ARGS(&backBuffers[bufferId]));

        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
        renderTargetViewDesc.Format = BACK_BUFFER_FORMAT;
        renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        device->CreateRenderTargetView(backBuffers[bufferId], &renderTargetViewDesc, backBufferCPUDescriptors[bufferId]);
    }

    bufferIndex = swapChain->GetCurrentBackBufferIndex();

    WaitForGPU(resourceCommandQueueId);
}

void Graphics::DirectX12Renderer::WaitForGPU(CommandQueueID _commandQueueId)
{
    auto commandQueue = commandManager->GetQueue(_commandQueueId);

    DirectX12Utilities::WaitForGPU(commandQueue, fence, fenceEvent, fenceValues[bufferIndex]);
}

void Graphics::DirectX12Renderer::PrepareToNextFrame()
{
    const uint64_t currentFenceValue = fenceValues[bufferIndex];

    auto commandQueue = commandManager->GetQueue(commandQueueId);
    commandQueue->Signal(fence, currentFenceValue);

    bufferIndex = swapChain->GetCurrentBackBufferIndex();

    if (fence->GetCompletedValue() < fenceValues[bufferIndex])
    {
        fence->SetEventOnCompletion(fenceValues[bufferIndex], fenceEvent);
        WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
    }

    fenceValues[bufferIndex] = currentFenceValue + 1;
}
