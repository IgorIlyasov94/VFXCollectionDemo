#include "ConstantBufferFactory.h"

Graphics::Resources::ConstantBufferFactory::ConstantBufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager)
	: _bufferManager(bufferManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::ConstantBufferFactory::~ConstantBufferFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::ConstantBufferFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& bufferDesc = static_cast<const BufferDesc*>(desc)[0];

	auto isDynamic = bufferDesc.flag == BufferFlag::IS_CONSTANT_DYNAMIC;

	auto bufferAllocationType = isDynamic ? BufferAllocationType::DYNAMIC_CONSTANT : BufferAllocationType::VERTEX_CONSTANT;
	auto bufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), bufferAllocationType);

	auto startAddress = bufferDesc.data.data();
	auto endAddress = startAddress + bufferDesc.data.size();

	if (isDynamic)
		std::copy(startAddress, endAddress, bufferAllocation.cpuAddress);
	else
	{
		auto uploadBufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UPLOAD);

		std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

		bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

		auto srcResource = uploadBufferAllocation.resource->GetResource();
		auto destResource = bufferAllocation.resource->GetResource();
		auto srcOffset = uploadBufferAllocation.resourceOffset;
		auto destOffset = bufferAllocation.resourceOffset;

		commandList->CopyBufferRegion(destResource, destOffset, srcResource, srcOffset, bufferDesc.data.size());

		bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	auto newConstantBuffer = new ConstantBuffer;
	newConstantBuffer->resource = bufferAllocation.resource;
	newConstantBuffer->size = bufferAllocation.size;
	newConstantBuffer->resourceCPUAddress = bufferAllocation.cpuAddress;
	newConstantBuffer->resourceGPUAddress = bufferAllocation.gpuAddress;
	newConstantBuffer->cbvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc{};
	viewDesc.BufferLocation = bufferAllocation.gpuAddress;
	viewDesc.SizeInBytes = static_cast<uint32_t>(bufferDesc.data.size() + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1u) &
		~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1u);

	device->CreateConstantBufferView(&viewDesc, newConstantBuffer->cbvDescriptor.cpuDescriptor);

	return static_cast<IResource*>(newConstantBuffer);
}
