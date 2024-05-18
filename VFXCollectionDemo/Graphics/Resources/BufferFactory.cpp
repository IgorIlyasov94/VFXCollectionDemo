#include "BufferFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::BufferFactory::BufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager)
	: _bufferManager(bufferManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::BufferFactory::~BufferFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::BufferFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& bufferDesc = static_cast<const BufferDesc*>(desc)[0];

	auto uploadBufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UPLOAD);

	auto startAddress = bufferDesc.data.data();
	auto endAddress = startAddress + bufferDesc.data.size();

	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	auto bufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::COMMON);

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

	auto srcResource = uploadBufferAllocation.resource->GetResource();
	auto destResource = bufferAllocation.resource->GetResource();
	auto srcOffset = uploadBufferAllocation.resourceOffset;
	auto destOffset = bufferAllocation.resourceOffset;

	commandList->CopyBufferRegion(destResource, destOffset, srcResource, srcOffset, bufferDesc.data.size());

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COMMON);

	auto newBuffer = new Buffer;
	newBuffer->resource = bufferAllocation.resource;
	newBuffer->size = bufferAllocation.size;
	newBuffer->resourceGPUAddress = bufferAllocation.gpuAddress;
	newBuffer->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(bufferDesc);
	
	device->CreateShaderResourceView(destResource, &srvDesc, newBuffer->srvDescriptor.cpuDescriptor);

	return static_cast<IResource*>(newBuffer);
}
