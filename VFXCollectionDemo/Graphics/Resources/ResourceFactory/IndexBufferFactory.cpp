#include "IndexBufferFactory.h"

Memory::IndexBufferFactory::IndexBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator)
	: _device(device), _bufferAllocator(bufferAllocator)
{

}

Memory::ResourceData Memory::IndexBufferFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	auto tempBufferAllocator = _bufferAllocator.lock();

	if (tempBufferAllocator == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Buffer Allocator is null!");

	if (_device == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Device is null!");

	if (commandList == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Command List is null!");

	BufferAllocation indexBufferAllocation{};
	BufferAllocation uploadBufferAllocation{};
	ID3D12Resource* resource = desc.preparedResource;

	if (resource == nullptr)
	{
		BufferAllocator::AllocationDesc allocationDesc{};
		allocationDesc.size = desc.dataSize;
		allocationDesc.alignment = 64 * Graphics::_KB;
		allocationDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

		tempBufferAllocator->Allocate(_device, allocationDesc, indexBufferAllocation);

		allocationDesc.heapType = D3D12_HEAP_TYPE_UPLOAD;

		tempBufferAllocator->AllocateTemporary(_device, allocationDesc, uploadBufferAllocation);

		if (indexBufferAllocation.bufferResource == nullptr)
			throw std::exception("IndexBufferFactory::CreateResource: Index Buffer Resource is null!");

		if (uploadBufferAllocation.bufferResource == nullptr)
			throw std::exception("IndexBufferFactory::CreateResource: Upload Buffer Resource is null!");

		auto startAddress = reinterpret_cast<const uint8_t*>(desc.data);
		auto endAddress = reinterpret_cast<const uint8_t*>(desc.data) + desc.dataSize;
		std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

		commandList->CopyBufferRegion(indexBufferAllocation.bufferResource, indexBufferAllocation.gpuPageOffset,
			uploadBufferAllocation.bufferResource, 0, uploadBufferAllocation.bufferResource->GetDesc().Width);

		Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), indexBufferAllocation.bufferResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}
	else
	{
		indexBufferAllocation.gpuAddress = resource->GetGPUVirtualAddress();
		indexBufferAllocation.gpuPageOffset = 0;
		indexBufferAllocation.nonAlignedSizeInBytes = desc.dataSize;
		indexBufferAllocation.bufferResource = resource;

		Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), indexBufferAllocation.bufferResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}

	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexBufferAllocation.gpuAddress;
	indexBufferView.SizeInBytes = static_cast<uint32_t>(desc.dataSize);
	indexBufferView.Format = (desc.dataStride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	Memory::ResourceData resourceData{};

	resourceData.indicesCount = static_cast<uint32_t>(desc.dataSize / desc.dataStride);
	resourceData.bufferAllocation = std::make_unique<Memory::BufferAllocation>(std::move(indexBufferAllocation));
	resourceData.indexBufferView = std::make_unique<D3D12_INDEX_BUFFER_VIEW>(std::move(indexBufferView));
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;

	return resourceData;
}