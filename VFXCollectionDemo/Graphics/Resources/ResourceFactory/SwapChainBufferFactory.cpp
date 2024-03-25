#include "SwapChainBufferFactory.h"

Memory::SwapChainBufferFactory::SwapChainBufferFactory(ID3D12Device* device, std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::SwapChainBufferFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("SwapChainBufferFactory::CreateResource: Device is null!");

	if (desc.textureInfo == nullptr)
		throw std::exception("SwapChainBufferFactory::CreateResource: TextureInfo is null!");

	if (desc.preparedResource == nullptr)
		throw std::exception("SwapChainBufferFactory::CreateResource: SwapChain resource is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("SwapChainBufferFactory::CreateResource: Descriptor Allocator is null!");

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = desc.textureInfo->format;

	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
	renderTargetViewDesc.Format = desc.textureInfo->format;
	renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	DescriptorAllocation renderTargetDescriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, renderTargetDescriptorAllocation);

	_device->CreateRenderTargetView(desc.preparedResource, &renderTargetViewDesc,
		renderTargetDescriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};

	resourceData.renderTargetViewDesc = std::make_unique<D3D12_RENDER_TARGET_VIEW_DESC>(renderTargetViewDesc);
	resourceData.textureInfo = desc.textureInfo;
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_PRESENT;
	resourceData.renderTargetDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(renderTargetDescriptorAllocation));
	
	return resourceData;
}
