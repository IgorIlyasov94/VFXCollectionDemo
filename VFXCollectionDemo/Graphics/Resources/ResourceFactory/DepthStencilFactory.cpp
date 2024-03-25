#include "DepthStencilFactory.h"

Memory::DepthStencilFactory::DepthStencilFactory(ID3D12Device* device, std::weak_ptr<TextureAllocator> textureAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _textureAllocator(textureAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::DepthStencilFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("DepthStencilFactory::CreateResource: Device is null!");

	if (desc.textureInfo == nullptr)
		throw std::exception("DepthStencilFactory::CreateResource: TextureInfo is null!");

	auto tempTextureAllocator = _textureAllocator.lock();

	if (tempTextureAllocator == nullptr)
		throw std::exception("DepthStencilFactory::CreateResource: Texture Allocator is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("DepthStencilFactory::CreateResource: Descriptor Allocator is null!");

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = (desc.depthBit == 32) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	TextureAllocation textureAllocation{};
	tempTextureAllocator->Allocate(_device, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue, *desc.textureInfo, textureAllocation);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = (desc.depthBit == 32) ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shaderResourceViewDesc.Texture2D.MipLevels = desc.textureInfo->mipLevels;

	_device->CreateShaderResourceView(textureAllocation.textureResource, &shaderResourceViewDesc, descriptorAllocation.descriptorBase);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = (desc.depthBit == 32) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	DescriptorAllocation depthStencilDescriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, depthStencilDescriptorAllocation);

	_device->CreateDepthStencilView(textureAllocation.textureResource, &depthStencilViewDesc,
		depthStencilDescriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};

	resourceData.shaderResourceViewDesc = std::make_unique<D3D12_SHADER_RESOURCE_VIEW_DESC>(shaderResourceViewDesc);
	resourceData.depthStencilViewDesc = std::make_unique<D3D12_DEPTH_STENCIL_VIEW_DESC>(depthStencilViewDesc);
	resourceData.textureInfo = desc.textureInfo;
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	resourceData.descriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.depthStencilDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(depthStencilDescriptorAllocation));
	resourceData.textureAllocation = std::make_unique<TextureAllocation>(std::move(textureAllocation));

	return resourceData;
}
