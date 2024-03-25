#include "VertexBufferFactory.h"

Memory::VertexBufferFactory::VertexBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator)
	: _device(device), _bufferAllocator(bufferAllocator)
{

}

Memory::ResourceData Memory::VertexBufferFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("VertexBufferFactory::CreateResource: Device is null!");

	auto tempBufferAllocator = _bufferAllocator.lock();
	if (tempBufferAllocator == nullptr)
		throw std::exception("VertexBufferFactory::CreateResource: Buffer Allocator is null!");

	if (commandList == nullptr && !desc.isDynamic)
		throw std::exception("VertexBufferFactory::CreateResource: Command List is null!");

	BufferAllocation vertexBufferAllocation{};
	BufferAllocation uploadBufferAllocation{};
	ID3D12Resource* resource = desc.preparedResource;

	if (resource == nullptr)
	{
		BufferAllocator::AllocationDesc allocationDesc{};
		allocationDesc.size = desc.dataSize;
		allocationDesc.alignment = 64 * Graphics::_KB;
		allocationDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

		if (!desc.isDynamic)
		{
			tempBufferAllocator->Allocate(_device, allocationDesc, vertexBufferAllocation);

			if (vertexBufferAllocation.bufferResource == nullptr)
				throw std::exception("VertexBufferFactory::CreateResource: Vertex Buffer Resource is null!");
		}

		allocationDesc.heapType = D3D12_HEAP_TYPE_UPLOAD;

		if (desc.isDynamic)
			tempBufferAllocator->Allocate(_device, allocationDesc, uploadBufferAllocation);
		else
			tempBufferAllocator->AllocateTemporary(_device, allocationDesc, uploadBufferAllocation);

		if (uploadBufferAllocation.bufferResource == nullptr)
			throw std::exception("VertexBufferFactory::CreateResource: Upload Buffer Resource is null!");

		auto startAddress = reinterpret_cast<const uint8_t*>(desc.data);
		auto endAddress = reinterpret_cast<const uint8_t*>(desc.data) + desc.dataSize;
		std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

		if (!desc.isDynamic)
		{
			commandList->CopyBufferRegion(vertexBufferAllocation.bufferResource, vertexBufferAllocation.gpuPageOffset,
				uploadBufferAllocation.bufferResource, 0, uploadBufferAllocation.bufferResource->GetDesc().Width);

			Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), vertexBufferAllocation.bufferResource,
				D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
	}
	else
	{
		vertexBufferAllocation.gpuAddress = resource->GetGPUVirtualAddress();
		vertexBufferAllocation.gpuPageOffset = 0;
		vertexBufferAllocation.nonAlignedSizeInBytes = desc.dataSize;
		vertexBufferAllocation.bufferResource = resource;

		Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), vertexBufferAllocation.bufferResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = (desc.isDynamic) ? uploadBufferAllocation.gpuAddress : vertexBufferAllocation.gpuAddress;
	vertexBufferView.SizeInBytes = static_cast<uint32_t>(desc.dataSize);
	vertexBufferView.StrideInBytes = static_cast<uint32_t>(desc.dataStride);

	Memory::ResourceData resourceData{};

	resourceData.bufferAllocation = (desc.isDynamic && resource == nullptr) ?
		std::make_unique<Memory::BufferAllocation>(std::move(uploadBufferAllocation)) :
		std::make_unique<Memory::BufferAllocation>(std::move(vertexBufferAllocation));

	resourceData.vertexBufferView = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>(std::move(vertexBufferView));
	resourceData.currentResourceState = (desc.isDynamic && resource == nullptr) ?
		D3D12_RESOURCE_STATE_GENERIC_READ :
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	return resourceData;
}
