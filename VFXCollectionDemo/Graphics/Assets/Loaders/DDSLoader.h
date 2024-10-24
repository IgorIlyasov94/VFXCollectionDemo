#pragma once

#include "../../Resources/IResourceDesc.h"

namespace Graphics::Assets::Loaders
{
	enum class DDSFormat : uint32_t
	{
		R8_UNORM = 0u,
		R8G8B8A8_UNORM = 1u
	};

	struct DDSSaveDesc
	{
	public:
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		DDSFormat targetFormat;
		D3D12_SRV_DIMENSION dimension;
	};

	class DDSLoader final
	{
	public:
		static void Load(std::filesystem::path filePath, Resources::TextureDesc& textureDesc);
		static void Save(std::filesystem::path filePath, const DDSSaveDesc& saveDesc, const std::vector<floatN>& data);

	private:
		DDSLoader() = delete;
		~DDSLoader() = delete;
		DDSLoader(const DDSLoader&) = delete;
		DDSLoader(DDSLoader&&) = delete;
		DDSLoader& operator=(const DDSLoader&) = delete;
		DDSLoader& operator=(DDSLoader&&) = delete;

		struct DDSPixelFormat
		{
		public:
			uint32_t size;
			uint32_t flags;
			uint32_t fourCC;
			uint32_t rgbBitCount;
			uint32_t rBitMask;
			uint32_t gBitMask;
			uint32_t bBitMask;
			uint32_t aBitMask;
		};

		struct DDSHeader
		{
		public:
			uint32_t fileCode;
			uint32_t headerSize;
			uint32_t flags;
			uint32_t height;
			uint32_t width;
			uint32_t pitchOrLinearSize;
			uint32_t depth;
			uint32_t mipMapCount;
			uint32_t reserved1[11];
			DDSPixelFormat pixelFormat;
			uint32_t caps[4];
			uint32_t reserved2;
		};

		struct DDSHeaderDXT10
		{
		public:
			DXGI_FORMAT format;
			D3D12_RESOURCE_DIMENSION dimension;
			uint32_t miscFlag;
			uint32_t arraySize;
			uint32_t reserved;
		};

		static constexpr uint32_t MakeFourCC(const char&& ch0, const char&& ch1, const char&& ch2, const char&& ch3) noexcept;
		static constexpr bool CheckBitMask(const DDSPixelFormat& format, uint32_t x, uint32_t y, uint32_t z, uint32_t w) noexcept;
		static DXGI_FORMAT GetFormat(const DDSPixelFormat& format) noexcept;
		static DXGI_FORMAT GetFormat(DDSFormat format) noexcept;
		static DDSPixelFormat GetPixelFormat(DDSFormat format) noexcept;
		static uint32_t CalculatePitch(uint32_t width, DDSFormat format) noexcept;
		static void Convert(const DDSSaveDesc& desc, const std::vector<floatN>& data, std::vector<uint8_t>& convertedData);
		
		static inline uint8_t Float32ToUNorm8(float value);
		
		static constexpr uint32_t DDSD_CAPS = 0x1u;
		static constexpr uint32_t DDSD_HEIGHT = 0x2u;
		static constexpr uint32_t DDSD_WIDTH = 0x4u;
		static constexpr uint32_t DDSD_PITCH = 0x8u;
		static constexpr uint32_t DDSD_PIXELFORMAT = 0x1000u;
		static constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000u;
		static constexpr uint32_t DDSD_LINEARSIZE = 0x80000u;
		static constexpr uint32_t DDSD_DEPTH = 0x800000u;

		static constexpr uint32_t DDSCAPS_COMPLEX = 0x8u;
		static constexpr uint32_t DDSCAPS_MIPMAP = 0x400000u;
		static constexpr uint32_t DDSCAPS_TEXTURE = 0x1000u;

		static constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x400u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000u;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000u;
		static constexpr uint32_t DDSCAPS2_VOLUME = 0x200000u;

		static constexpr uint32_t DDPF_ALPHAPIXELS = 0x1u;
		static constexpr uint32_t DDPF_ALPHA = 0x2u;
		static constexpr uint32_t DDPF_FOURCC = 0x4u;
		static constexpr uint32_t DDPF_RGB = 0x40u;
		static constexpr uint32_t DDPF_YUV = 0x200u;
		static constexpr uint32_t DDPF_LUMINANCE = 0x20000u;
		static constexpr uint32_t DDPF_BUMPDUDV = 0x00080000u;
	};
}
