#pragma once

#include "IResourceFactory.h"

namespace Memory
{
	class ConstantBufferFactory : public IResourceFactory
	{
	public:
		ConstantBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator,
			std::weak_ptr<DescriptorAllocator> descriptorAllocator);

		ResourceData CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
			const ResourceDesc& desc) override final;

	private:
		ID3D12Device* _device;
		std::weak_ptr<BufferAllocator> _bufferAllocator;
		std::weak_ptr<DescriptorAllocator> _descriptorAllocator;
	};
}
