#include "RenderTargetFactory.h"

Memory::RenderTargetFactory::RenderTargetFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _textureAllocator(textureAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::RenderTargetFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("RenderTargetFactory::CreateResource: Device is null!");

	if (desc.textureInfo == nullptr)
		throw std::exception("RenderTargetFactory::CreateResource: TextureInfo is null!");

	auto tempTextureAllocator = _textureAllocator.lock();

	if (tempTextureAllocator == nullptr)
		throw std::exception("RenderTargetFactory::CreateResource: Texture Allocator is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("RenderTargetFactory::CreateResource: Descriptor Allocator is null!");

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = desc.textureInfo->format;

	TextureAllocation textureAllocation{};
	tempTextureAllocator->Allocate(_device, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue, *desc.textureInfo, textureAllocation);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = desc.textureInfo->format;
	shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shaderResourceViewDesc.Texture2D.MipLevels = desc.textureInfo->mipLevels;

	_device->CreateShaderResourceView(textureAllocation.textureResource, &shaderResourceViewDesc, descriptorAllocation.descriptorBase);

	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
	renderTargetViewDesc.Format = desc.textureInfo->format;
	renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	DescriptorAllocation renderTargetDescriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, renderTargetDescriptorAllocation);

	_device->CreateRenderTargetView(textureAllocation.textureResource, &renderTargetViewDesc,
		renderTargetDescriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};

	resourceData.shaderResourceViewDesc = std::make_unique<D3D12_SHADER_RESOURCE_VIEW_DESC>(shaderResourceViewDesc);
	resourceData.renderTargetViewDesc = std::make_unique<D3D12_RENDER_TARGET_VIEW_DESC>(renderTargetViewDesc);
	resourceData.textureInfo = desc.textureInfo;
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceData.descriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.renderTargetDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(renderTargetDescriptorAllocation));
	resourceData.textureAllocation = std::make_unique<TextureAllocation>(std::move(textureAllocation));

	return resourceData;
}
