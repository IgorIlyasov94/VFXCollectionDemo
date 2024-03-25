#pragma once

#include "IResourceFactory.h"

namespace Memory
{
	class TextureFactory : public IResourceFactory
	{
	public:
		TextureFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
			std::weak_ptr<DescriptorAllocator> descriptorAllocator);

		ResourceData CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
			const ResourceDesc& desc) override final;

	private:
		D3D12_SHADER_RESOURCE_VIEW_DESC SetSRVDesc(const Graphics::TextureInfo& info);
		void UploadTexture(ID3D12GraphicsCommandList* commandList, ID3D12Resource* uploadBuffer, ID3D12Resource* targetTexture,
			const Graphics::TextureInfo& textureInfo, void* data, uint8_t* uploadBufferCPUAddress);
		void CopyRawDataToSubresource(const Graphics::TextureInfo& srcTextureInfo, uint32_t numRows, uint16_t numSlices,
			uint64_t destRowPitch, uint64_t destSlicePitch, uint64_t rowSizeInBytes, const uint8_t* srcAddress, uint8_t* destAddress);

		ID3D12Device* _device;
		std::weak_ptr<TextureAllocator> _textureAllocator;
		std::weak_ptr<DescriptorAllocator> _descriptorAllocator;
	};
}
