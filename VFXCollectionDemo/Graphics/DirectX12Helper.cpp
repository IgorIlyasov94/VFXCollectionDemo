#include "DirectX12Helper.h"

Graphics::DirectX12Helper::~DirectX12Helper()
{

}

void Graphics::DirectX12Helper::CreateFactory(IDXGIFactory7** factory)
{
	UINT dxgiFactoryFlags = 0;

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory)),
		"DirectX12Helper::CreateFactory: Factory creating failed!");
}

void Graphics::DirectX12Helper::CreateDevice(IDXGIAdapter1* adapter, ID3D12Device** device)
{
	ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device)),
		"DirectX12Helper::CreateDevice: Device creating failed!");
}

void Graphics::DirectX12Helper::CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** commandQueue)
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue)),
		"DirectX12Helper::CreateCommandQueue: Command Queue creating failed!");
}

void Graphics::DirectX12Helper::CreateSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND& windowHandler,
	const uint32_t buffersCount, const int32_t& resolutionX, const int32_t& resolutionY, IDXGISwapChain1** swapChain)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = buffersCount;
	swapChainDesc.Width = resolutionX;
	swapChainDesc.Height = resolutionY;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, windowHandler, &swapChainDesc, nullptr, nullptr, swapChain),
		"DirectX12Helper::CreateSwapChain: Swap Chain creating failed!");
}

void Graphics::DirectX12Helper::CreateDescriptorHeap(ID3D12Device* device, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
	D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap** descriptorHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = flags;
	descriptorHeapDesc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap)),
		"DirectX12Helper::CreateDescriptorHeap: Descriptor Heap creating failed!");
}

void Graphics::DirectX12Helper::CreateStandardSamplerDescs(std::vector<D3D12_STATIC_SAMPLER_DESC>& samplerDescs)
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;

	samplerDescs.push_back(samplerDesc);

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.ShaderRegister = 1;

	samplerDescs.push_back(samplerDesc);

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ShaderRegister = 2;

	samplerDescs.push_back(samplerDesc);

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.ShaderRegister = 3;

	samplerDescs.push_back(samplerDesc);

	samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ShaderRegister = 4;

	samplerDescs.push_back(samplerDesc);

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ShaderRegister = 5;

	samplerDescs.push_back(samplerDesc);
}

void Graphics::DirectX12Helper::GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter4** adapter)
{
	*adapter = nullptr;

	ComPtr<IDXGIAdapter4> adapter4;
	
	for (auto adapterId = 0;
		factory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter4)) != DXGI_ERROR_NOT_FOUND;
		adapterId++)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter4.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			break;
	}

	*adapter = adapter4.Detach();
}

void Graphics::DirectX12Helper::SetupRasterizerDesc(D3D12_RASTERIZER_DESC& rasterizerDesc, D3D12_CULL_MODE cullMode) noexcept
{
	rasterizerDesc.AntialiasedLineEnable = false;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	rasterizerDesc.CullMode = cullMode;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
}

void Graphics::DirectX12Helper::SetupBlendDesc(D3D12_BLEND_DESC& blendDesc, bool blendOn, D3D12_BLEND srcBlend, D3D12_BLEND destBlend,
	D3D12_BLEND_OP blendOp, D3D12_BLEND srcBlendAlpha, D3D12_BLEND destBlendAlpha, D3D12_BLEND_OP blendOpAlpha) noexcept
{
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	const D3D12_RENDER_TARGET_BLEND_DESC rtBlendDescDefault =
	{
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};

	for (auto renderTargetId = 0; renderTargetId < 8; renderTargetId++)
	{
		blendDesc.RenderTarget[renderTargetId] = rtBlendDescDefault;
	}

	blendDesc.RenderTarget[0].BlendEnable = blendOn;
	blendDesc.RenderTarget[0].SrcBlend = srcBlend;
	blendDesc.RenderTarget[0].DestBlend = destBlend;
	blendDesc.RenderTarget[0].BlendOp = blendOp;
	blendDesc.RenderTarget[0].SrcBlendAlpha = srcBlendAlpha;
	blendDesc.RenderTarget[0].DestBlendAlpha = destBlendAlpha;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendOpAlpha;
}

void Graphics::DirectX12Helper::SetupDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& depthStencilDesc, bool depthEnable) noexcept
{
	depthStencilDesc.DepthEnable = depthEnable;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	const D3D12_DEPTH_STENCILOP_DESC depthStencilOpDesc =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
	};

	depthStencilDesc.BackFace = depthStencilOpDesc;
	depthStencilDesc.FrontFace = depthStencilOpDesc;
}

void Graphics::DirectX12Helper::SetupResourceBufferDesc(D3D12_RESOURCE_DESC& resourceDesc, uint64_t bufferSize,
	D3D12_RESOURCE_FLAGS resourceFlag, uint64_t alignment) noexcept
{
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Alignment = alignment;
	resourceDesc.Flags = resourceFlag;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Width = (bufferSize + 255Ui64) & ~255Ui64;
}

void Graphics::DirectX12Helper::SetupResourceTextureDesc(D3D12_RESOURCE_DESC& resourceDesc, const TextureInfo& textureInfo,
	D3D12_RESOURCE_FLAGS resourceFlag, uint64_t alignment) noexcept
{
	resourceDesc.Dimension = textureInfo.dimension;
	resourceDesc.DepthOrArraySize = textureInfo.depth;
	resourceDesc.Alignment = 0;
	resourceDesc.Flags = resourceFlag;
	resourceDesc.Format = textureInfo.format;
	resourceDesc.Height = textureInfo.height;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.MipLevels = textureInfo.mipLevels;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Width = textureInfo.width;
}

void Graphics::DirectX12Helper::SetupHeapProperties(D3D12_HEAP_PROPERTIES& heapProperties, D3D12_HEAP_TYPE heapType) noexcept
{
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.VisibleNodeMask = 1;
	heapProperties.Type = heapType;
}

void Graphics::DirectX12Helper::SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* const resource,
	D3D12_RESOURCE_BARRIER_FLAGS barrierFlags, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	if (stateBefore == stateAfter)
		return;

	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = barrierFlags;
	resourceBarrier.Transition.pResource = resource;
	resourceBarrier.Transition.StateBefore = stateBefore;
	resourceBarrier.Transition.StateAfter = stateAfter;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &resourceBarrier);
}

void Graphics::DirectX12Helper::ExecuteGPUCommands(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* allocator,
	ID3D12CommandQueue* queue, ID3D12Fence* fence, HANDLE& fenceEvent, uint64_t& fenceValue)
{
	ThrowIfFailed(commandList->Close(), "ExecuteGPUCommands: Command List closing error!");

	ID3D12CommandList* commandLists[] = { commandList };

	queue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(queue->Signal(fence, fenceValue), "ExecuteGPUCommands: Signal error!");
	ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent), "ExecuteGPUCommands: Fence error!");

	WaitForSingleObjectEx(fenceEvent, INFINITE, false);

	fenceValue++;

	ThrowIfFailed(allocator->Reset(), "ExecuteGPUCommands: Command Allocator resetting error!");
	ThrowIfFailed(commandList->Reset(allocator, nullptr), "ExecuteGPUCommands: Command List resetting error!");
}
