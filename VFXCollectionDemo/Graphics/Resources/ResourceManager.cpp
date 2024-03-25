#include "ResourceManager.h"

Memory::ResourceManager::ResourceManager(ID3D12Device* _device)
	: device(_device), fenceValue(0)
{
	Graphics::DirectX12Helper::CreateCommandQueue(device, &commandQueue);

	ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
		"ResourceManager: Fence creating error!");
	fenceValue++;

	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (fenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "ResourceManager::Initialize: Fence Event creating error!");

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
		"ResourceManager::Initialize: Command Allocator creating error!");

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&commandList)), "ResourceManager::Initialize: Command List creating error!");

	descriptorAllocator = std::make_shared<DescriptorAllocator>();
	bufferAllocator = std::make_shared<BufferAllocator>();
	textureAllocator = std::make_shared<TextureAllocator>();

	resourceFactories.push_back(new VertexBufferFactory(device, bufferAllocator));
	resourceFactories.push_back(new IndexBufferFactory(device, bufferAllocator));
	resourceFactories.push_back(new ConstantBufferFactory(device, bufferAllocator, descriptorAllocator));
	resourceFactories.push_back(new TextureFactory(device, textureAllocator, descriptorAllocator));
	resourceFactories.push_back(new BufferFactory(device, bufferAllocator, descriptorAllocator));
	resourceFactories.push_back(new SamplerFactory(device, descriptorAllocator));
	resourceFactories.push_back(new RenderTargetFactory(device, textureAllocator, descriptorAllocator));
	resourceFactories.push_back(new DepthStencilFactory(device, textureAllocator, descriptorAllocator));
	resourceFactories.push_back(new RWTextureFactory(device, textureAllocator, descriptorAllocator));
	resourceFactories.push_back(new RWBufferFactory(device, bufferAllocator, descriptorAllocator));
	resourceFactories.push_back(new SwapChainBufferFactory(device, descriptorAllocator));
}

Memory::ResourceManager::~ResourceManager()
{
	
}

Memory::ResourceId Memory::ResourceManager::CreateResource(ResourceType resourceType, const ResourceDesc& desc)
{
	auto resourceTypeIndex = static_cast<uint8_t>(resourceType);

	size_t id;

	if (freePlaceIndices.empty())
	{
		id = resources.size();
		resources.push_back(resourceFactories[resourceTypeIndex]->CreateResource(commandList, resourceType, desc));
	}
	else
	{
		id = freePlaceIndices.front();
		freePlaceIndices.pop();

		resources[id] = resourceFactories[resourceTypeIndex]->CreateResource(commandList, resourceType, desc);
	}

	return { id , resourceType };
}

void Memory::ResourceManager::ReleaseResource(const ResourceId& resourceId)
{
	auto id = resourceId.GetId();

	resources[id].Release();
	freePlaceIndices.push(id);
}

void Memory::ResourceManager::UpdateResources()
{
	Graphics::DirectX12Helper::ExecuteGPUCommands(commandList.Get(), commandAllocator.Get(),
		commandQueue.Get(), fence.Get(), fenceEvent, fenceValue);

	bufferAllocator->ReleaseTemporaryBuffers();
	textureAllocator->ReleaseTemporaryBuffers();
}

const Memory::ResourceData& Memory::ResourceManager::GetResourceData(const ResourceId& resourceId) const
{
	return resources[resourceId.GetId()];
}

D3D12_VERTEX_BUFFER_VIEW Memory::ResourceManager::GetVertexBufferView(const ResourceId& resourceId) const
{
	return *resources[resourceId.GetId()].vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Memory::ResourceManager::GetIndexBufferView(const ResourceId& resourceId) const
{
	return *resources[resourceId.GetId()].indexBufferView;
}

D3D12_CPU_DESCRIPTOR_HANDLE Memory::ResourceManager::GetRenderTargetDescriptorBase(const ResourceId& resourceId) const
{
	auto& descriptorAllocation = resources[resourceId.GetId()].renderTargetDescriptorAllocation;

	if (descriptorAllocation != nullptr)
		return descriptorAllocation->descriptorBase;

	return {};
}

D3D12_CPU_DESCRIPTOR_HANDLE Memory::ResourceManager::GetDepthStencilDescriptorBase(const ResourceId& resourceId) const
{
	auto& descriptorAllocation = resources[resourceId.GetId()].depthStencilDescriptorAllocation;

	if (descriptorAllocation != nullptr)
		return descriptorAllocation->descriptorBase;

	return {};
}

ID3D12DescriptorHeap* Memory::ResourceManager::GetCurrentCbvSrvUavDescriptorHeap()
{
	return descriptorAllocator->GetCurrentCbvSrvUavDescriptorHeap();
}

ID3D12DescriptorHeap* Memory::ResourceManager::GetCurrentSamplerDescriptorHeap()
{
	return descriptorAllocator->GetCurrentSamplerDescriptorHeap();
}

void Memory::ResourceManager::UpdateBuffer(const ResourceId& resourceId, const void* data, size_t dataSize)
{
	std::copy(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + dataSize,
		resources[resourceId.GetId()].bufferAllocation->cpuAddress);
}

void Memory::ResourceManager::SetResourceBarrier(ID3D12GraphicsCommandList* _commandList, const ResourceId& resourceId,
	D3D12_RESOURCE_BARRIER_FLAGS resourceBarrierFlags, D3D12_RESOURCE_STATES newState)
{
	auto& resourceData = resources[resourceId.GetId()];
	
	if (resourceData.currentResourceState == newState)
		return;

	auto& resource = resourceData.textureInfo == nullptr ?
		resourceData.bufferAllocation->bufferResource :
		resourceData.textureAllocation->textureResource;

	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = resourceBarrierFlags;
	resourceBarrier.Transition.pResource = resource;
	resourceBarrier.Transition.StateBefore = resourceData.currentResourceState;
	resourceBarrier.Transition.StateAfter = newState;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &resourceBarrier);

	if (resourceBarrierFlags != D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY)
		resourceData.currentResourceState = newState;
}

void Memory::ResourceManager::SetUAVBarrier(ID3D12GraphicsCommandList* _commandList, const ResourceId& resourceId)
{
	auto& resourceData = resources[resourceId.GetId()];

	auto& resource = resourceData.textureInfo == nullptr ?
		resourceData.bufferAllocation->bufferResource :
		resourceData.textureAllocation->textureResource;

	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.UAV.pResource = resource;
	
	commandList->ResourceBarrier(1, &resourceBarrier);
}
