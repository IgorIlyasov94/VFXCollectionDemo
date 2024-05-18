#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../TextureManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class TextureFactory : public IResourceFactory
	{
	public:
		TextureFactory(TextureManager* textureManager, DescriptorManager* descriptorManager);
		~TextureFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		TextureFactory() = delete;

		void UploadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12Resource* uploadBuffer,
			ID3D12Resource* targetTexture, const TextureDesc& desc, uint8_t* uploadBufferCPUAddress);

		void CopyRawDataToSubresource(const TextureDesc& srcTextureDesc, uint32_t numRows, uint16_t numSlices,
			uint64_t destRowPitch, uint64_t destSlicePitch, uint64_t rowSizeInBytes, const uint8_t* srcAddress, uint8_t* destAddress);

		TextureManager* _textureManager;
		DescriptorManager* _descriptorManager;
	};
}
