#include "VertexBufferFactory.h"

Graphics::Resources::VertexBufferFactory::VertexBufferFactory(BufferManager* bufferManager)
	: _bufferManager(bufferManager)
{

}

Graphics::Resources::VertexBufferFactory::~VertexBufferFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::VertexBufferFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& bufferDesc = static_cast<const BufferDesc*>(desc)[0];
	
	auto uploadBufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UPLOAD);

	auto startAddress = bufferDesc.data.data();
	auto endAddress = startAddress + bufferDesc.data.size();

	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	auto bufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::VERTEX_CONSTANT);

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

	auto srcResource = uploadBufferAllocation.resource->GetResource();
	auto destResource = bufferAllocation.resource->GetResource();
	auto srcOffset = uploadBufferAllocation.resourceOffset;
	auto destOffset = bufferAllocation.resourceOffset;

	commandList->CopyBufferRegion(destResource, destOffset, srcResource, srcOffset, bufferDesc.data.size());

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	auto newVertexBuffer = new VertexBuffer;
	newVertexBuffer->resource = bufferAllocation.resource;
	newVertexBuffer->viewDesc.BufferLocation = bufferAllocation.gpuAddress;
	newVertexBuffer->viewDesc.SizeInBytes = static_cast<uint32_t>(bufferDesc.data.size());
	newVertexBuffer->viewDesc.StrideInBytes = bufferDesc.dataStride;

	return static_cast<IResource*>(newVertexBuffer);
}
