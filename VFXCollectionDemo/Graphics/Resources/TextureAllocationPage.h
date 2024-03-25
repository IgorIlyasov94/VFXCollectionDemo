#pragma once

#include "..\..\Includes.h"
#include "..\DirectX12Helper.h"
#include "..\..\Common\CommonUtilities.h"

namespace Memory
{
	struct TextureAllocation
	{
	public:
		TextureAllocation() = default;
		~TextureAllocation() {};
		TextureAllocation(TextureAllocation&& source) noexcept;

		uint8_t* cpuAddress;
		ID3D12Resource* textureResource;
	};

	class TextureAllocationPage
	{
	public:
		using PageDesc = struct
		{
			D3D12_HEAP_TYPE heapType;
			D3D12_RESOURCE_FLAGS resourceFlags;
			const D3D12_CLEAR_VALUE* clearValue;
		};

		TextureAllocationPage(ID3D12Device* device, PageDesc& desc, const Graphics::TextureInfo& textureInfo);
		~TextureAllocationPage();

		void GetAllocation(TextureAllocation& allocation);

	private:
		uint8_t* cpuAddress;

		D3D12_HEAP_TYPE heapType;

		ComPtr<ID3D12Resource> pageResource;
	};
}