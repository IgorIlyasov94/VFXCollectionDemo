#pragma once

#include "IResourceFactory.h"

namespace Memory
{
	class IndexBufferFactory : public IResourceFactory
	{
	public:
		IndexBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator);

		ResourceData CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
			const ResourceDesc& desc) override final;

	private:
		ID3D12Device* _device;
		std::weak_ptr<BufferAllocator> _bufferAllocator;
	};
}
