#include "TextureManager.h"

Graphics::TextureManager::TextureManager()
{

}

Graphics::TextureManager::~TextureManager()
{
	for (auto& resource : resources)
		delete resource;

	ReleaseUploadBuffers();
}

Graphics::TextureAllocation Graphics::TextureManager::Allocate(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags,
	const D3D12_CLEAR_VALUE& clearValue, const Resources::TextureDesc& desc)
{
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CreationNodeMask = 1u;
	heapProperties.VisibleNodeMask = 1u;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = desc.dimension;
	resourceDesc.DepthOrArraySize = desc.depth;
	resourceDesc.Flags = resourceFlags;
	resourceDesc.Format = desc.format;
	resourceDesc.Height = desc.height;
	resourceDesc.MipLevels = desc.mipLevels;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Width = desc.width;

	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;

	if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	else if ((resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	TextureAllocation allocation{};

	ID3D12Resource* resource = nullptr;

	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initState, &clearValue,
		IID_PPV_ARGS(&resource));

	allocation.resource = new Resources::GPUResource(resource, initState);

	resources.push_back(allocation.resource);

	return allocation;
}

Graphics::TextureAllocation Graphics::TextureManager::AllocateUploadBuffer(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags,
	const D3D12_CLEAR_VALUE& clearValue, const Resources::TextureDesc& desc)
{
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CreationNodeMask = 1u;
	heapProperties.VisibleNodeMask = 1u;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = desc.dimension;
	resourceDesc.DepthOrArraySize = desc.depth;
	resourceDesc.Flags = resourceFlags;
	resourceDesc.Format = desc.format;
	resourceDesc.Height = desc.height;
	resourceDesc.MipLevels = desc.mipLevels;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Width = desc.width;

	uint64_t requiredUploadBufferSize;
	uint32_t numSubresources = desc.depth * desc.mipLevels;

	device->GetCopyableFootprints(&resourceDesc, 0u, numSubresources, 0u, nullptr, nullptr, nullptr, &requiredUploadBufferSize);

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1;
	resourceDesc.Width = (requiredUploadBufferSize + 255Ui64) & ~255Ui64;

	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ;

	TextureAllocation allocation{};

	ID3D12Resource* resource = nullptr;

	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initState, &clearValue,
		IID_PPV_ARGS(&resource));

	allocation.resource = new Resources::GPUResource(resource, initState);
	allocation.cpuAddress = allocation.resource->Map();

	uploadBuffers.push_back(allocation.resource);

	return allocation;
}

void Graphics::TextureManager::Deallocate(Resources::GPUResource* allocatedResource)
{
	for (auto& resource : resources)
		if (resource == allocatedResource)
		{
			delete resource;
			std::swap(resource, resources.back());

			resources.resize(resources.size() - 1u);

			return;
		}
}

void Graphics::TextureManager::ReleaseUploadBuffers()
{
	for (auto& buffer : uploadBuffers)
	{
		buffer->Unmap();
		delete buffer;
	}

	uploadBuffers.clear();
}
