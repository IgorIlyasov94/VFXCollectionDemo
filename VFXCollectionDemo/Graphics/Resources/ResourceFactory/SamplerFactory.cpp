#include "SamplerFactory.h"

Memory::SamplerFactory::SamplerFactory(ID3D12Device* device, std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::SamplerFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("SamplerFactory::CreateResource: Descriptor Allocator is null!");

	if (_device == nullptr)
		throw std::exception("SamplerFactory::CreateResource: Device is null!");

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);

	_device->CreateSampler(desc.samplerDesc.get(), descriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};

	resourceData.samplerDesc = std::make_unique<D3D12_SAMPLER_DESC>();
	*resourceData.samplerDesc = *desc.samplerDesc;
	resourceData.descriptorAllocation = std::make_unique<Memory::DescriptorAllocation>(std::move(descriptorAllocation));

	return resourceData;
}
