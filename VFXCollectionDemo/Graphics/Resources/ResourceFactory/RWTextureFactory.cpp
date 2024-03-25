#include "RWTextureFactory.h"

Memory::RWTextureFactory::RWTextureFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _textureAllocator(textureAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::RWTextureFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("RWTextureFactory::CreateResource: Device is null!");

	if (desc.textureInfo == nullptr)
		throw std::exception("RWTextureFactory::CreateResource: TextureInfo is null!");

	auto tempTextureAllocator = _textureAllocator.lock();

	if (tempTextureAllocator == nullptr)
		throw std::exception("RWTextureFactory::CreateResource: Texture Allocator is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("RWTextureFactory::CreateResource: Descriptor Allocator is null!");

	if (commandList == nullptr)
		throw std::exception("RWTextureFactory::CreateResource: Command List is null!");

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = desc.textureInfo->format;
	
	TextureAllocation textureAllocation{};
	tempTextureAllocator->Allocate(_device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &clearValue, *desc.textureInfo, textureAllocation);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
	SetSRVDesc(*desc.textureInfo, shaderResourceViewDesc, unorderedAccessViewDesc);

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);
	_device->CreateShaderResourceView(textureAllocation.textureResource, &shaderResourceViewDesc, descriptorAllocation.descriptorBase);

	DescriptorAllocation unorderedAccessDescriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, unorderedAccessDescriptorAllocation);
	_device->CreateUnorderedAccessView(textureAllocation.textureResource, nullptr, &unorderedAccessViewDesc,
		unorderedAccessDescriptorAllocation.descriptorBase);

	descriptorAllocationDesc.isUAVShaderNonVisible = true;

	DescriptorAllocation shaderNonVisibleDescriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, shaderNonVisibleDescriptorAllocation);
	_device->CreateUnorderedAccessView(textureAllocation.textureResource, nullptr, &unorderedAccessViewDesc,
		shaderNonVisibleDescriptorAllocation.descriptorBase);

	Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), textureAllocation.textureResource,
		D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	Memory::ResourceData resourceData{};
	resourceData.shaderResourceViewDesc = std::make_unique<D3D12_SHADER_RESOURCE_VIEW_DESC>(std::move(shaderResourceViewDesc));
	resourceData.unorderedAccessViewDesc = std::make_unique<D3D12_UNORDERED_ACCESS_VIEW_DESC>(std::move(unorderedAccessViewDesc));
	resourceData.textureInfo = desc.textureInfo;
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	resourceData.descriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.shaderNonVisibleDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(shaderNonVisibleDescriptorAllocation));
	resourceData.unorderedDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(unorderedAccessDescriptorAllocation));
	resourceData.textureAllocation = std::make_unique<TextureAllocation>(std::move(textureAllocation));

	return resourceData;
}

void Memory::RWTextureFactory::SetSRVDesc(const Graphics::TextureInfo& textureInfo, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
	D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc)
{
	srvDesc = {};
	srvDesc.Format = textureInfo.format;
	srvDesc.ViewDimension = textureInfo.srvDimension;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uavDesc = {};
	uavDesc.Format = textureInfo.format;

	if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
	{
		srvDesc.Texture1D.MipLevels = 1;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	}
	else if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
	{
		srvDesc.Texture2D.MipLevels = 1;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	}
	else if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
	{
		srvDesc.Texture3D.MipLevels = 1;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.WSize = textureInfo.depth;
	}
	else if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		srvDesc.Texture1DArray.MipLevels = 1;
		srvDesc.Texture1DArray.ArraySize = textureInfo.depth;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		uavDesc.Texture1DArray.ArraySize = textureInfo.depth;
	}
	else if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.ArraySize = textureInfo.depth;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = textureInfo.depth;
	}
}
