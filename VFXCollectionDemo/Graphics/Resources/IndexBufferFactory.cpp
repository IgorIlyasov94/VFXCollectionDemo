#include "IndexBufferFactory.h"

Graphics::Resources::IndexBufferFactory::IndexBufferFactory(BufferManager* bufferManager)
	: _bufferManager(bufferManager)
{

}

Graphics::Resources::IndexBufferFactory::~IndexBufferFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::IndexBufferFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& bufferDesc = static_cast<const BufferDesc*>(desc)[0];

	auto uploadBufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::UPLOAD);

	auto startAddress = bufferDesc.data.data();
	auto endAddress = startAddress + bufferDesc.data.size();

	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	auto bufferAllocation = _bufferManager->Allocate(device, bufferDesc.data.size(), BufferAllocationType::INDEX);

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

	auto srcResource = uploadBufferAllocation.resource->GetResource();
	auto destResource = bufferAllocation.resource->GetResource();
	auto srcOffset = uploadBufferAllocation.resourceOffset;
	auto destOffset = bufferAllocation.resourceOffset;

	commandList->CopyBufferRegion(destResource, destOffset, srcResource, srcOffset, bufferDesc.data.size());

	bufferAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	auto newIndexBuffer = new IndexBuffer;
	newIndexBuffer->resource = bufferAllocation.resource;
	newIndexBuffer->viewDesc.BufferLocation = bufferAllocation.gpuAddress;
	newIndexBuffer->viewDesc.SizeInBytes = static_cast<uint32_t>(bufferDesc.data.size());
	newIndexBuffer->viewDesc.Format = bufferDesc.dataStride == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	newIndexBuffer->indicesNumber = bufferDesc.numElements;

	return static_cast<IResource*>(newIndexBuffer);
}
