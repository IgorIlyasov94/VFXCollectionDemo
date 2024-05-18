#include "TextureFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::TextureFactory::TextureFactory(TextureManager* textureManager, DescriptorManager* descriptorManager)
	: _textureManager(textureManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::TextureFactory::~TextureFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::TextureFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& textureDesc = static_cast<const TextureDesc*>(desc)[0];
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = textureDesc.format;

	auto textureAllocation = _textureManager->Allocate(device, D3D12_RESOURCE_FLAG_NONE, clearValue, textureDesc);
	auto uploadTextureAllocation = _textureManager->AllocateUploadBuffer(device, D3D12_RESOURCE_FLAG_NONE, clearValue, textureDesc);

	textureAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

	auto srcResource = uploadTextureAllocation.resource->GetResource();
	auto destResource = textureAllocation.resource->GetResource();
	UploadTexture(device, commandList, srcResource, destResource, textureDesc, textureAllocation.cpuAddress);

	textureAllocation.resource->Barrier(commandList, D3D12_RESOURCE_STATE_COMMON);

	auto texture = new Texture;
	std::swap(texture->resource, textureAllocation.resource);
	texture->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(textureDesc);

	device->CreateShaderResourceView(destResource, &srvDesc, texture->srvDescriptor.cpuDescriptor);

	return static_cast<IResource*>(texture);
}

void Graphics::Resources::TextureFactory::UploadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ID3D12Resource* uploadBuffer, ID3D12Resource* targetTexture, const TextureDesc& desc, uint8_t* uploadBufferCPUAddress)
{
	uint32_t numSubresources = desc.depth * desc.mipLevels;

	uint32_t layoutsSize = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(uint32_t) + sizeof(uint64_t)) * numSubresources;

	std::vector<uint8_t> srcLayouts;
	srcLayouts.resize(layoutsSize);

	auto targetTextureDesc = targetTexture->GetDesc();

	std::vector<uint32_t> numRows;
	numRows.resize(numSubresources);

	std::vector<uint64_t> rowSizesPerByte;
	rowSizesPerByte.resize(numSubresources);

	uint64_t requiredSize;

	device->GetCopyableFootprints(&targetTextureDesc, 0, numSubresources, 0,
		reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(srcLayouts.data()),
		numRows.data(), rowSizesPerByte.data(), &requiredSize);

	for (uint32_t subresourceIndex = 0; subresourceIndex < numSubresources; subresourceIndex++)
	{
		auto srcLayout = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(&srcLayouts[0])[subresourceIndex];
		auto destRowPitch = static_cast<uint64_t>(srcLayout.Footprint.RowPitch) * numRows[subresourceIndex];

		CopyRawDataToSubresource(desc, numRows[subresourceIndex], srcLayout.Footprint.Depth, srcLayout.Footprint.RowPitch,
			destRowPitch, rowSizesPerByte[subresourceIndex], desc.data.data() + srcLayout.Offset * 0,
			uploadBufferCPUAddress);
	}

	for (uint32_t subresourceIndex = 0; subresourceIndex < numSubresources; subresourceIndex++)
	{
		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource = uploadBuffer;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(&srcLayouts[0])[subresourceIndex];

		D3D12_TEXTURE_COPY_LOCATION destLocation{};
		destLocation.pResource = targetTexture;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destLocation.SubresourceIndex = subresourceIndex;

		commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
}

void Graphics::Resources::TextureFactory::CopyRawDataToSubresource(const TextureDesc& srcTextureDesc, uint32_t numRows, uint16_t numSlices,
	uint64_t destRowPitch, uint64_t destSlicePitch, uint64_t rowSizeInBytes, const uint8_t* srcAddress, uint8_t* destAddress)
{
	for (uint32_t sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
	{
		for (uint64_t rowIndex = 0; rowIndex < numRows; rowIndex++)
		{
			const uint8_t* srcBeginAddress = srcAddress + sliceIndex * srcTextureDesc.slicePitch + rowIndex * srcTextureDesc.rowPitch;
			const uint8_t* srcEndAddress = srcBeginAddress + rowSizeInBytes;
			uint8_t* destBeginAddress = destAddress + sliceIndex * destSlicePitch + rowIndex * destRowPitch;

			std::copy(srcBeginAddress, srcEndAddress, destBeginAddress);
		}
	}
}