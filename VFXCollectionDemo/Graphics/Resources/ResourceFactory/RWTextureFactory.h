#pragma once

#include "IResourceFactory.h"

namespace Memory
{
	class RWTextureFactory : public IResourceFactory
	{
	public:
		RWTextureFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
			std::weak_ptr<DescriptorAllocator> descriptorAllocator);

		ResourceData CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
			const ResourceDesc& desc) override final;

	private:
		void SetSRVDesc(const Graphics::TextureInfo& textureInfo, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
			D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc);

		ID3D12Device* _device;
		std::weak_ptr<TextureAllocator> _textureAllocator;
		std::weak_ptr<DescriptorAllocator> _descriptorAllocator;
	};
}
