#pragma once

#include "../DirectX12Includes.h"

namespace Graphics::Resources
{
	enum class BufferFlag : uint32_t
	{
		NONE = 0u,
		ADD_COUNTER = 1u,
		IS_CONSTANT_DYNAMIC = 2u
	};

	struct IResourceDesc
	{
	public:
		virtual ~IResourceDesc() = 0 {};
	};

	struct BufferDesc : public IResourceDesc
	{
	public:
		uint32_t dataStride;

		BufferFlag flag;

		uint32_t numElements;

		DXGI_FORMAT format;

		std::vector<uint8_t> data;
	};

	struct TextureDesc : public IResourceDesc
	{
	public:
		uint64_t width;
		uint32_t height;
		uint32_t depth;

		uint32_t mipLevels;

		uint64_t rowPitch;
		uint64_t slicePitch;

		uint32_t depthBit;

		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		D3D12_SRV_DIMENSION srvDimension;

		std::vector<uint8_t> data;
	};
}
