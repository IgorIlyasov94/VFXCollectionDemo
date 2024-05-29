#include "RWBufferFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::RWBufferFactory::RWBufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager)
	: _bufferManager(bufferManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::RWBufferFactory::~RWBufferFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::RWBufferFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& bufferDesc = static_cast<const BufferDesc*>(desc)[0];

	auto bufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UNORDERED_ACCESS);
	auto uploadBufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UPLOAD);

	auto startAddress = bufferDesc.data.data();
	auto endAddress = startAddress + bufferDesc.data.size();

	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

	auto srcResource = uploadBufferAllocation.resource->GetResource();
	auto destResource = bufferAllocation.resource->GetResource();
	commandList->CopyBufferRegion(destResource, bufferAllocation.resourceOffset, srcResource,
		uploadBufferAllocation.resourceOffset, srcResource->GetDesc().Width);

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	auto rwBuffer = new RWBuffer;
	std::swap(rwBuffer->resource, bufferAllocation.resource);
	rwBuffer->counterResource = nullptr;
	rwBuffer->size = bufferAllocation.size;
	rwBuffer->resourceGPUAddress = bufferAllocation.gpuAddress;
	rwBuffer->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	rwBuffer->uavDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	rwBuffer->uavNonShaderVisibleDescriptor = {};
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(bufferDesc);
	auto uavDesc = DirectX12Utilities::CreateUAVDesc(bufferDesc);
	
	if (bufferDesc.flag == BufferFlag::RAW)
	{
		rwBuffer->uavNonShaderVisibleDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV_NON_SHADER_VISIBLE);
		device->CreateUnorderedAccessView(destResource, nullptr, &uavDesc, rwBuffer->uavNonShaderVisibleDescriptor.cpuDescriptor);
	}
	else if (bufferDesc.flag == BufferFlag::ADD_COUNTER)
	{
		auto counterAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UNORDERED_ACCESS);
		rwBuffer->counterResource = counterAllocation.resource;
	}

	device->CreateShaderResourceView(destResource, &srvDesc, rwBuffer->srvDescriptor.cpuDescriptor);
	device->CreateUnorderedAccessView(destResource, nullptr, &uavDesc, rwBuffer->uavDescriptor.cpuDescriptor);
	
	return static_cast<IResource*>(rwBuffer);
}
