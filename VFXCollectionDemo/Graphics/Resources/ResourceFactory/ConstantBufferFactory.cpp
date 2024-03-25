#include "ConstantBufferFactory.h"

Memory::ConstantBufferFactory::ConstantBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _bufferAllocator(bufferAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::ConstantBufferFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	auto tempBufferAllocator = _bufferAllocator.lock();

	if (tempBufferAllocator == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Buffer Allocator is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Descriptor Allocator is null!");

	if (_device == nullptr)
		throw std::exception("IndexBufferFactory::CreateResource: Device is null!");

	BufferAllocator::AllocationDesc allocationDesc{};
	allocationDesc.size = desc.dataSize;
	allocationDesc.alignment = 64 * Graphics::_KB;
	allocationDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

	BufferAllocation constantBufferAllocation{};
	tempBufferAllocator->Allocate(_device, allocationDesc, constantBufferAllocation);

	allocationDesc.heapType = D3D12_HEAP_TYPE_UPLOAD;

	BufferAllocation uploadBufferAllocation{};
	tempBufferAllocator->AllocateTemporary(_device, allocationDesc, uploadBufferAllocation);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);

	if (constantBufferAllocation.bufferResource == nullptr)
		throw std::exception("ConstantBufferFactory::CreateResource: Index Buffer Resource is null!");

	if (uploadBufferAllocation.bufferResource == nullptr)
		throw std::exception("ConstantBufferFactory::CreateResource: Upload Buffer Resource is null!");

	if (descriptorAllocation.descriptorHeap == nullptr)
		throw std::exception("ConstantBufferFactory::CreateResource: Descriptor Heap is null!");

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc{};
	constantBufferViewDesc.BufferLocation = constantBufferAllocation.gpuAddress;
	constantBufferViewDesc.SizeInBytes = static_cast<UINT>(Common::CommonUtility::AlignSize(desc.dataSize,
		static_cast<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
	
	_device->CreateConstantBufferView(&constantBufferViewDesc, descriptorAllocation.descriptorBase);

	auto startAddress = reinterpret_cast<const uint8_t*>(desc.data);
	auto endAddress = reinterpret_cast<const uint8_t*>(desc.data) + desc.dataSize;
	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	Memory::ResourceData resourceData{};

	resourceData.bufferAllocation = std::make_unique<Memory::BufferAllocation>(std::move(constantBufferAllocation));
	resourceData.descriptorAllocation = std::make_unique<Memory::DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.constantBufferViewDesc = std::make_unique<D3D12_CONSTANT_BUFFER_VIEW_DESC>(std::move(constantBufferViewDesc));
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

	return resourceData;
}
