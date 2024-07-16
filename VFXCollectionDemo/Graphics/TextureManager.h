#pragma once

#include "DirectX12Includes.h"
#include "Resources/IResourceDesc.h"
#include "Resources/GPUResource.h"

namespace Graphics
{
	struct TextureAllocation
	{
	public:
		Resources::GPUResource* resource;
		uint8_t* cpuAddress;
	};

	class TextureManager
	{
	public:
		TextureManager();
		~TextureManager();

		TextureAllocation Allocate(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_CLEAR_VALUE& clearValue,
			const Resources::TextureDesc& desc);
		TextureAllocation AllocateUploadBuffer(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_CLEAR_VALUE& clearValue,
			const Resources::TextureDesc& desc);

		void Deallocate(Resources::GPUResource* allocatedResource);

		void ReleaseTempBuffers();

	private:
		TextureManager(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager& operator=(TextureManager&&) = delete;

		std::vector<Resources::GPUResource*> resources;
		std::vector<Resources::GPUResource*> uploadBuffers;
	};
}
