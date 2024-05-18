#include "DDSLoader.h"

void Graphics::Assets::Loaders::DDSLoader::Load(std::filesystem::path filePath, Resources::TextureDesc& textureDesc)
{
    if (!std::filesystem::exists(filePath))
        return;

    std::ifstream ddsFile(filePath, std::ios::binary);
    ddsFile.seekg(0, std::ios::end);

    size_t textureSizeInBytes = static_cast<size_t>(ddsFile.tellg());

    if (textureSizeInBytes < sizeof(DDSHeader))
        return;

    textureSizeInBytes -= sizeof(DDSHeader);

    ddsFile.seekg(0, std::ios::beg);

    DDSHeader header{};

    ddsFile.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));

    bool hasHeaderDXT10 = false;
    DDSHeaderDXT10 headerDXT10{};

    textureDesc.width = std::max(header.width, 1u);
    textureDesc.height = std::max(header.height, 1u);
    textureDesc.depth = std::max(header.depth, 1u);
    textureDesc.mipLevels = std::max(header.mipMapCount, 1u);
    textureDesc.rowPitch = std::max(header.pitchOrLinearSize, 1u);
    textureDesc.slicePitch = textureDesc.rowPitch * textureDesc.height;

    if (MakeFourCC('D', 'X', '1', '0') == header.pixelFormat.fourCC)
    {
        if (textureSizeInBytes < sizeof(DDSHeaderDXT10))
            return;

        textureSizeInBytes -= sizeof(DDSHeaderDXT10);

        hasHeaderDXT10 = true;
        ddsFile.read(reinterpret_cast<char*>(&headerDXT10), sizeof(DDSHeaderDXT10));

        textureDesc.format = static_cast<DXGI_FORMAT>(headerDXT10.format);
        textureDesc.dimension = static_cast<D3D12_RESOURCE_DIMENSION>(headerDXT10.dimension);

        auto arraySize = headerDXT10.arraySize;

        if (arraySize > 1u)
            textureDesc.depth = arraySize;

        if (textureDesc.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        {
            if (arraySize > 1u)
                textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            else
                textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        }
        else if (textureDesc.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            if ((headerDXT10.miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE) > 0u)
                if (arraySize > 1u)
                    textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                else
                    textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            else
                if (arraySize > 1u)
                    textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                else
                    textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        }
        else if (textureDesc.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
            textureDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    }
    else
    {
        textureDesc.format = GetFormat(header.pixelFormat);
        textureDesc.dimension = header.depth > 1u ? D3D12_RESOURCE_DIMENSION_TEXTURE3D :
            header.height > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        textureDesc.srvDimension = header.depth > 1u ? D3D12_SRV_DIMENSION_TEXTURE3D :
            header.height > 1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE1D;
    }

    textureDesc.data.resize(textureSizeInBytes);
    ddsFile.read(reinterpret_cast<char*>(textureDesc.data.data()), textureSizeInBytes);
}

constexpr uint32_t Graphics::Assets::Loaders::DDSLoader::MakeFourCC(const char&& ch0, const char&& ch1,
	const char&& ch2, const char&& ch3) noexcept
{
	return static_cast<uint32_t>(ch0) | static_cast<uint32_t>(ch1) << 8 |
		static_cast<uint32_t>(ch2) << 16 | static_cast<uint32_t>(ch3) << 24;
}

constexpr bool Graphics::Assets::Loaders::DDSLoader::CheckBitMask(const DDSPixelFormat& format,
	uint32_t x, uint32_t y, uint32_t z, uint32_t w) noexcept
{
	return format.rBitMask == x && format.gBitMask == y && format.bBitMask == z && format.aBitMask == w;
}

DXGI_FORMAT Graphics::Assets::Loaders::DDSLoader::GetFormat(const DDSPixelFormat& format) noexcept
{
    if ((format.flags & 0x40u) > 0)
    {
        if (format.rgbBitCount == 32)
        {
            if (CheckBitMask(format, 0x000000ffu, 0x0000ff00u, 0x00ff0000u, 0xff000000u))
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            if (CheckBitMask(format, 0x00ff0000u, 0x0000ff00u, 0x000000ffu, 0xff000000u))
                return DXGI_FORMAT_B8G8R8A8_UNORM;

            if (CheckBitMask(format, 0x00ff0000u, 0x0000ff00u, 0x000000ffu, 0x00000000u))
                return DXGI_FORMAT_B8G8R8X8_UNORM;

            if (CheckBitMask(format, 0x3ff00000u, 0x000ffc00u, 0x000003ffu, 0xc0000000u))
                return DXGI_FORMAT_R10G10B10A2_UNORM;

            if (CheckBitMask(format, 0x0000ffffu, 0xffff0000u, 0x00000000u, 0x00000000u))
                return DXGI_FORMAT_R16G16_UNORM;

            if (CheckBitMask(format, 0xffffffffu, 0xffff0000u, 0x00000000u, 0x00000000u))
                return DXGI_FORMAT_R32_FLOAT;
        }
        else if (format.rgbBitCount == 16)
        {
            if (CheckBitMask(format, 0x7c00u, 0x03e0u, 0x001fu, 0x8000u))
                return DXGI_FORMAT_B5G5R5A1_UNORM;

            if (CheckBitMask(format, 0xf800u, 0x07e0u, 0x001fu, 0x0000u))
                return DXGI_FORMAT_B5G6R5_UNORM;

            if (CheckBitMask(format, 0x0f00u, 0x00f0u, 0x000fu, 0xf000u))
                return DXGI_FORMAT_B4G4R4A4_UNORM;

            if (CheckBitMask(format, 0x00ffu, 0x0000u, 0x0000u, 0xff00u))
                return DXGI_FORMAT_R8G8_UNORM;

            if (CheckBitMask(format, 0xffffu, 0x0000u, 0x0000u, 0x0000u))
                return DXGI_FORMAT_R16_UNORM;
        }
        else if (format.rgbBitCount == 8)
        {
            if (CheckBitMask(format, 0xffu, 0x00u, 0x00u, 0x00u))
                return DXGI_FORMAT_R8_UNORM;
        }
    }
    else if ((format.flags & 0x20000u) > 0)
    {
        if (format.rgbBitCount == 16)
        {
            if (CheckBitMask(format, 0xffffu, 0x0000u, 0x0000u, 0x0000u))
                return DXGI_FORMAT_R16_UNORM;

            if (CheckBitMask(format, 0x00ffu, 0x0000u, 0x0000u, 0xff00u))
                return DXGI_FORMAT_R8G8_UNORM;
        }
        else if (format.rgbBitCount == 8)
        {
            if (CheckBitMask(format, 0xffu, 0x00u, 0x00u, 0x00u))
                return DXGI_FORMAT_R8_UNORM;

            if (CheckBitMask(format, 0x00ffu, 0x0000u, 0x0000u, 0xff00u))
                return DXGI_FORMAT_R8G8_UNORM;
        }
    }
    else if ((format.flags & 0x2u) > 0)
    {
        if (format.rgbBitCount == 8)
            return DXGI_FORMAT_A8_UNORM;
    }
    else if ((format.flags & 0x00080000u) > 0)
    {
        if (format.rgbBitCount == 32)
        {
            if (CheckBitMask(format, 0x000000ffu, 0x0000ff00u, 0x00ff0000u, 0xff000000u))
                return DXGI_FORMAT_R8G8B8A8_SNORM;

            if (CheckBitMask(format, 0x0000ffffu, 0xffff0000u, 0x00000000u, 0x00000000u))
                return DXGI_FORMAT_R16G16_SNORM;
        }
        else if (format.rgbBitCount == 16)
        {
            if (CheckBitMask(format, 0x00ffu, 0xff00u, 0x0000u, 0x0000u))
                return DXGI_FORMAT_R8G8_SNORM;
        }
    }
    else if ((format.flags & 0x4u) > 0)
    {
        if (MakeFourCC('D', 'X', 'T', '1') == format.fourCC)
            return DXGI_FORMAT_BC1_UNORM;

        if (MakeFourCC('D', 'X', 'T', '2') == format.fourCC)
            return DXGI_FORMAT_BC2_UNORM;

        if (MakeFourCC('D', 'X', 'T', '3') == format.fourCC)
            return DXGI_FORMAT_BC2_UNORM;

        if (MakeFourCC('D', 'X', 'T', '4') == format.fourCC)
            return DXGI_FORMAT_BC3_UNORM;

        if (MakeFourCC('D', 'X', 'T', '5') == format.fourCC)
            return DXGI_FORMAT_BC3_UNORM;

        if (MakeFourCC('A', 'T', 'I', '1') == format.fourCC)
            return DXGI_FORMAT_BC4_UNORM;

        if (MakeFourCC('B', 'C', '4', 'U') == format.fourCC)
            return DXGI_FORMAT_BC4_UNORM;

        if (MakeFourCC('B', 'C', '4', 'S') == format.fourCC)
            return DXGI_FORMAT_BC4_UNORM;

        if (MakeFourCC('A', 'T', 'I', '2') == format.fourCC)
            return DXGI_FORMAT_BC5_UNORM;

        if (MakeFourCC('B', 'C', '5', 'U') == format.fourCC)
            return DXGI_FORMAT_BC5_UNORM;

        if (MakeFourCC('B', 'C', '5', 'S') == format.fourCC)
            return DXGI_FORMAT_BC5_SNORM;

        if (MakeFourCC('R', 'G', 'B', 'G') == format.fourCC)
            return DXGI_FORMAT_R8G8_B8G8_UNORM;

        if (MakeFourCC('G', 'R', 'G', 'B') == format.fourCC)
            return DXGI_FORMAT_G8R8_G8B8_UNORM;

        if (MakeFourCC('Y', 'U', 'Y', '2') == format.fourCC)
            return DXGI_FORMAT_YUY2;

        if (format.fourCC == 32)
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        if (format.fourCC == 110)
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        if (format.fourCC == 111)
            return DXGI_FORMAT_R16_FLOAT;

        if (format.fourCC == 112)
            return DXGI_FORMAT_R16G16_FLOAT;

        if (format.fourCC == 113)
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        if (format.fourCC == 114)
            return DXGI_FORMAT_R32_FLOAT;

        if (format.fourCC == 115)
            return DXGI_FORMAT_R32G32_FLOAT;

        if (format.fourCC == 116)
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    return DXGI_FORMAT_UNKNOWN;
}
