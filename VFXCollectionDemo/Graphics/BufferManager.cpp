#include "BufferManager.h"

Graphics::BufferManager::BufferManager()
{

}

Graphics::BufferManager::~BufferManager()
{
	for (auto& buffer : buffers)
	{
		if (buffer.resource == nullptr)
			continue;

		if (buffer.type == BufferAllocationType::DYNAMIC_CONSTANT)
			buffer.resource->Unmap();

		delete buffer.resource;
	}

	ReleaseUploadBuffers();
}

Graphics::BufferAllocation Graphics::BufferManager::Allocate(ID3D12Device* device, uint64_t size, BufferAllocationType type)
{
	BufferAllocation allocation{};
	allocation.size = size;

	auto alignedSize = AlignValue(size, DEFAULT_BUFFER_ALIGNMENT);

	if (type != BufferAllocationType::UPLOAD &&
		type != BufferAllocationType::COMMON &&
		type != BufferAllocationType::UNORDERED_ACCESS)
		for (auto& buffer : buffers)
		{
			if (buffer.type != type)
				continue;

			auto alignedOffset = AlignValue(buffer.chunk.offset, DEFAULT_BUFFER_ALIGNMENT);

			if ((alignedOffset + alignedSize) <= buffer.chunk.size)
			{
				allocation.resource = buffer.resource;

				for (auto& chunk : buffer.freeSpace)
					if (chunk.size <= alignedSize)
					{
						allocation.cpuAddress = buffer.cpuStartAddress + chunk.offset;
						allocation.gpuAddress = buffer.gpuStartAddress + chunk.offset;
						allocation.resourceOffset = chunk.offset;

						if ((chunk.size - alignedSize) < DEFAULT_BUFFER_ALIGNMENT)
						{
							std::swap(chunk, buffer.freeSpace.back());
							buffer.freeSpace.resize(buffer.freeSpace.size() - 1u);
						}
						else
							chunk.size = AlignValue(chunk.size - alignedSize, DEFAULT_BUFFER_ALIGNMENT);

						return allocation;
					}

				allocation.cpuAddress = buffer.cpuStartAddress + alignedOffset;
				allocation.gpuAddress = buffer.gpuStartAddress + alignedOffset;
				allocation.resourceOffset = alignedOffset;
				
				buffer.chunk.offset = alignedOffset + alignedSize;

				return allocation;
			}
		}

	D3D12_HEAP_PROPERTIES heapProperties{};

	heapProperties.Type = (type == BufferAllocationType::UPLOAD || type == BufferAllocationType::DYNAMIC_CONSTANT) ?
		D3D12_HEAP_TYPE_UPLOAD :
		D3D12_HEAP_TYPE_DEFAULT;

	heapProperties.CreationNodeMask = 1u;
	heapProperties.VisibleNodeMask = 1u;

	uint64_t resourceSize = (alignedSize > DEFAULT_BUFFER_SIZE || type == BufferAllocationType::UPLOAD) ?
		alignedSize :
		(type == BufferAllocationType::COMMON || type == BufferAllocationType::UNORDERED_ACCESS) ?
		SRV_BUFFER_SIZE :
		DEFAULT_BUFFER_SIZE;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Alignment = DEFAULT_BUFFER_ALIGNMENT;

	if (type == BufferAllocationType::UNORDERED_ACCESS)
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Width = resourceSize;

	auto flags = D3D12_HEAP_FLAG_NONE;
	auto state = (type == BufferAllocationType::UPLOAD || type == BufferAllocationType::DYNAMIC_CONSTANT) ?
		D3D12_RESOURCE_STATE_GENERIC_READ :
		(type == BufferAllocationType::UNORDERED_ACCESS) ?
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS :
		D3D12_RESOURCE_STATE_COPY_DEST;

	ID3D12Resource* resource{};
	device->CreateCommittedResource(&heapProperties, flags, &resourceDesc, state, nullptr, IID_PPV_ARGS(&resource));
	allocation.resource = new Resources::GPUResource(resource, state);

	if (type == BufferAllocationType::UPLOAD || type == BufferAllocationType::DYNAMIC_CONSTANT)
		allocation.cpuAddress = allocation.resource->Map();

	allocation.gpuAddress = resource->GetGPUVirtualAddress();
	
	BufferSpace newBufferSpace{};
	newBufferSpace.resource = allocation.resource;
	newBufferSpace.cpuStartAddress = allocation.cpuAddress;
	newBufferSpace.gpuStartAddress = allocation.gpuAddress;
	newBufferSpace.chunk.offset = alignedSize;
	newBufferSpace.chunk.size = resourceSize;
	newBufferSpace.type = type;

	if (type == BufferAllocationType::UPLOAD)
		uploadBuffers.push_back(std::move(newBufferSpace));
	else
		buffers.push_back(std::move(newBufferSpace));

	return allocation;
}

void Graphics::BufferManager::Deallocate(Resources::GPUResource* allocatedResource, D3D12_GPU_VIRTUAL_ADDRESS address, uint64_t size)
{
	for (auto& buffer : buffers)
		if (buffer.resource == allocatedResource)
		{
			auto alignedSize = AlignValue(size, DEFAULT_BUFFER_ALIGNMENT);
			
			if (buffer.chunk.size == alignedSize ||
				buffer.type == BufferAllocationType::COMMON ||
				buffer.type == BufferAllocationType::UNORDERED_ACCESS)
			{
				if (buffer.type == BufferAllocationType::DYNAMIC_CONSTANT)
					buffer.resource->Unmap();

				delete buffer.resource;
				buffer.resource = nullptr;

				std::swap(buffer, buffers.back());
				buffers.resize(buffers.size() - 1u);

				return;
			}

			auto gpuAddress = buffer.gpuStartAddress + buffer.chunk.offset - alignedSize;

			if (gpuAddress == address)
				buffer.chunk.offset -= alignedSize;
			else
			{
				BufferChunk freeChunk{};
				freeChunk.offset = address - buffer.gpuStartAddress;
				freeChunk.size = alignedSize;

				buffer.freeSpace.push_back(std::move(freeChunk));
			}

			return;
		}
}

void Graphics::BufferManager::ReleaseUploadBuffers()
{
	for (auto& buffer : uploadBuffers)
	{
		buffer.resource->Unmap();
		delete buffer.resource;
	}

	uploadBuffers.clear();
}

constexpr uint64_t Graphics::BufferManager::AlignValue(uint64_t value, uint64_t alignment)
{
	return (value + alignment - 1u) & ~(alignment - 1u);
}
