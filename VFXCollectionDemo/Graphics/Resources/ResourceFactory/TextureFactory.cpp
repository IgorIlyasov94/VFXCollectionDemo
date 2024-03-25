#include "TextureFactory.h"
#include "..\..\DirectX12Helper.h"

Memory::TextureFactory::TextureFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _textureAllocator(textureAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::TextureFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("TextureFactory::CreateResource: Device is null!");

	if (desc.textureInfo == nullptr)
		throw std::exception("TextureFactory::CreateResource: TextureInfo is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("TextureFactory::CreateResource: Descriptor Allocator is null!");

	auto resource = desc.preparedResource;
	TextureAllocation textureAllocation{};

	if (resource == nullptr)
	{
		auto tempTextureAllocator = _textureAllocator.lock();

		if (tempTextureAllocator == nullptr)
			throw std::exception("TextureFactory::CreateResource: Texture Allocator is null!");

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = desc.textureInfo->format;

		tempTextureAllocator->Allocate(_device, desc.resourceFlags, &clearValue, *desc.textureInfo, textureAllocation);

		TextureAllocation uploadTextureAllocation{};
		tempTextureAllocator->Allocate(_device, desc.resourceFlags, &clearValue, *desc.textureInfo, uploadTextureAllocation);

		Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), textureAllocation.textureResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		UploadTexture(commandList.Get(), uploadTextureAllocation.textureResource, textureAllocation.textureResource, *desc.textureInfo,
			desc.data, uploadTextureAllocation.cpuAddress);

		Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), textureAllocation.textureResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	}
	else
	{
		textureAllocation.textureResource = resource;
	}

	auto&& shaderResourceViewDesc = SetSRVDesc(*desc.textureInfo);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);

	_device->CreateShaderResourceView(resource, &shaderResourceViewDesc, descriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};

	resourceData.shaderResourceViewDesc = std::make_unique<D3D12_SHADER_RESOURCE_VIEW_DESC>(std::move(shaderResourceViewDesc));
	resourceData.textureInfo = std::forward<const std::shared_ptr<Graphics::TextureInfo>>(desc.textureInfo);
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_COMMON;
	resourceData.descriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.textureAllocation = std::make_unique<TextureAllocation>(std::move(textureAllocation));

	return resourceData;
}

D3D12_SHADER_RESOURCE_VIEW_DESC Memory::TextureFactory::SetSRVDesc(const Graphics::TextureInfo& textureInfo)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = textureInfo.format;
	shaderResourceViewDesc.ViewDimension = textureInfo.srvDimension;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	
	if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		shaderResourceViewDesc.Texture1D.MipLevels = textureInfo.mipLevels;
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		shaderResourceViewDesc.Texture1DArray.MipLevels = textureInfo.mipLevels;
		shaderResourceViewDesc.Texture1DArray.ArraySize = textureInfo.depth;
	}
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
		shaderResourceViewDesc.Texture2D.MipLevels = textureInfo.mipLevels;
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		shaderResourceViewDesc.Texture2DArray.MipLevels = textureInfo.mipLevels;
		shaderResourceViewDesc.Texture2DArray.ArraySize = textureInfo.depth;
	}
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
		shaderResourceViewDesc.TextureCube.MipLevels = textureInfo.mipLevels;
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
	{
		shaderResourceViewDesc.TextureCubeArray.MipLevels = textureInfo.mipLevels;
		shaderResourceViewDesc.TextureCubeArray.NumCubes = textureInfo.depth;
	}
	else if (textureInfo.srvDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
		shaderResourceViewDesc.Texture3D.MipLevels = textureInfo.mipLevels;

	return shaderResourceViewDesc;
}

void Memory::TextureFactory::UploadTexture(ID3D12GraphicsCommandList* commandList, ID3D12Resource* uploadBuffer, ID3D12Resource* targetTexture,
	const Graphics::TextureInfo& textureInfo, void* data, uint8_t* uploadBufferCPUAddress)
{
	uint32_t numSubresources = textureInfo.depth * textureInfo.mipLevels;

	std::vector<uint8_t> srcLayouts((sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(uint32_t) + sizeof(uint64_t)) * numSubresources);

	auto targetTextureDesc = targetTexture->GetDesc();
	std::vector<uint32_t> numRows(numSubresources);
	std::vector<uint64_t> rowSizesPerByte(numSubresources);
	uint64_t requiredSize;

	_device->GetCopyableFootprints(&targetTextureDesc, 0, numSubresources, 0, reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(srcLayouts.data()),
		numRows.data(), rowSizesPerByte.data(), &requiredSize);

	for (uint32_t subresourceIndex = 0; subresourceIndex < numSubresources; subresourceIndex++)
	{
		auto srcLayout = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(&srcLayouts[0])[subresourceIndex];
		auto destRowPitch = static_cast<uint64_t>(srcLayout.Footprint.RowPitch) * numRows[subresourceIndex];

		CopyRawDataToSubresource(textureInfo, numRows[subresourceIndex], srcLayout.Footprint.Depth, srcLayout.Footprint.RowPitch,
			destRowPitch, rowSizesPerByte[subresourceIndex], reinterpret_cast<const uint8_t*>(data) + srcLayout.Offset * 0, uploadBufferCPUAddress);
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

void Memory::TextureFactory::CopyRawDataToSubresource(const Graphics::TextureInfo& srcTextureInfo, uint32_t numRows, uint16_t numSlices,
	uint64_t destRowPitch, uint64_t destSlicePitch, uint64_t rowSizeInBytes, const uint8_t* srcAddress, uint8_t* destAddress)
{
	for (uint16_t sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
	{
		for (uint64_t rowIndex = 0; rowIndex < numRows; rowIndex++)
		{
			const uint8_t* srcBeginAddress = srcAddress + sliceIndex * srcTextureInfo.slicePitch + rowIndex * srcTextureInfo.rowPitch;
			const uint8_t* srcEndAddress = srcBeginAddress + rowSizeInBytes;
			uint8_t* destBeginAddress = destAddress + sliceIndex * destSlicePitch + rowIndex * destRowPitch;

			std::copy(srcBeginAddress, srcEndAddress, destBeginAddress);
		}
	}
}
