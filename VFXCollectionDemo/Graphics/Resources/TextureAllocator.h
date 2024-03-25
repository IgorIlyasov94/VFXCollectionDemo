#pragma once

#include "TextureAllocationPage.h"

namespace Memory
{
	class TextureAllocator
	{
	public:
		TextureAllocator() {};
		~TextureAllocator() {};

		void Allocate(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_CLEAR_VALUE* clearValue,
			const Graphics::TextureInfo& textureInfo, TextureAllocation& allocation);
		void AllocateTemporaryUpload(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags,
			const Graphics::TextureInfo& textureInfo, TextureAllocation& allocation);

		void ReleaseTemporaryBuffers();

	private:
		
		using TextureAllocationPagePool = std::deque<std::shared_ptr<TextureAllocationPage>>;

		TextureAllocationPagePool pages;
		TextureAllocationPagePool tempUploadPages;
	};
}