#include "RWBufferFactory.h"

Memory::RWBufferFactory::RWBufferFactory(ID3D12Device* device, std::weak_ptr<BufferAllocator> bufferAllocator,
	std::weak_ptr<DescriptorAllocator> descriptorAllocator)
	: _device(device), _bufferAllocator(bufferAllocator), _descriptorAllocator(descriptorAllocator)
{

}

Memory::ResourceData Memory::RWBufferFactory::CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type,
	const ResourceDesc& desc)
{
	if (_device == nullptr)
		throw std::exception("RWBufferFactory::CreateResource: Device is null!");

	auto tempBufferAllocator = _bufferAllocator.lock();

	if (tempBufferAllocator == nullptr)
		throw std::exception("RWBufferFactory::CreateResource: Buffer Allocator is null!");

	auto tempDescriptorAllocator = _descriptorAllocator.lock();

	if (tempDescriptorAllocator == nullptr)
		throw std::exception("RWBufferFactory::CreateResource: Descriptor Allocator is null!");

	if (commandList == nullptr)
		throw std::exception("RWBufferFactory::CreateResource: Command List is null!");

	BufferAllocator::AllocationDesc allocationDesc{};
	allocationDesc.size = desc.dataSize;
	allocationDesc.alignment = 64 * Graphics::_KB;
	allocationDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

	BufferAllocation bufferAllocation{};
	tempBufferAllocator->AllocateUnorderedAccess(_device, allocationDesc, bufferAllocation);

	allocationDesc.heapType = D3D12_HEAP_TYPE_UPLOAD;

	BufferAllocation uploadBufferAllocation{};
	tempBufferAllocator->AllocateTemporary(_device, allocationDesc, uploadBufferAllocation);

	if (bufferAllocation.bufferResource == nullptr)
		throw std::exception("BufferFactory::CreateResource: Index Buffer Resource is null!");

	if (uploadBufferAllocation.bufferResource == nullptr)
		throw std::exception("BufferFactory::CreateResource: Upload Buffer Resource is null!");

	auto startAddress = reinterpret_cast<const uint8_t*>(desc.data);
	auto endAddress = reinterpret_cast<const uint8_t*>(desc.data) + desc.dataSize;
	std::copy(startAddress, endAddress, uploadBufferAllocation.cpuAddress);

	Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), bufferAllocation.bufferResource,
		D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyBufferRegion(bufferAllocation.bufferResource, bufferAllocation.gpuPageOffset,
		uploadBufferAllocation.bufferResource, 0, uploadBufferAllocation.bufferResource->GetDesc().Width);

	Graphics::DirectX12Helper::SetResourceBarrier(commandList.Get(), bufferAllocation.bufferResource,
		D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	DescriptorAllocator::AllocationDesc descriptorAllocationDesc{};
	descriptorAllocationDesc.numDescriptors = 1;
	descriptorAllocationDesc.isUAVShaderNonVisible = false;
	descriptorAllocationDesc.descriptorType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = (desc.dataStride == 0) ? desc.bufferFormat : (desc.dataStride == 1) ?
		DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shaderResourceViewDesc.Buffer.Flags = (desc.dataStride == 1) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
	shaderResourceViewDesc.Buffer.NumElements = static_cast<uint32_t>(desc.numElements);
	shaderResourceViewDesc.Buffer.StructureByteStride = (desc.dataStride > 1) ? static_cast<uint32_t>(desc.dataStride) : 0u;

	D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
	unorderedAccessViewDesc.Format = (desc.dataStride == 0) ? desc.bufferFormat : (desc.dataStride == 1) ?
		DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
	unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	unorderedAccessViewDesc.Buffer.Flags = (desc.dataStride == 1) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
	unorderedAccessViewDesc.Buffer.NumElements = static_cast<uint32_t>(desc.numElements);
	unorderedAccessViewDesc.Buffer.StructureByteStride = (desc.dataStride > 1) ? static_cast<uint32_t>(desc.dataStride) : 0u;

	DescriptorAllocation descriptorAllocation{};
	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, descriptorAllocation);
	_device->CreateShaderResourceView(bufferAllocation.bufferResource, &shaderResourceViewDesc, descriptorAllocation.descriptorBase);

	BufferAllocation counterBufferAllocation{};
	DescriptorAllocation unorderedAccessDescriptorAllocation{};
	DescriptorAllocation shaderNonVisibleDescriptorAllocation{};

	if (desc.dataStride <= 1)
	{
		descriptorAllocationDesc.isUAVShaderNonVisible = true;

		tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, shaderNonVisibleDescriptorAllocation);

		_device->CreateUnorderedAccessView(bufferAllocation.bufferResource, nullptr, &unorderedAccessViewDesc,
			shaderNonVisibleDescriptorAllocation.descriptorBase);
	}
	else if (desc.addCounter)
	{
		allocationDesc.size = 4;
		allocationDesc.alignment = 64 * Graphics::_KB;
		allocationDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;

		tempBufferAllocator->AllocateUnorderedAccess(_device, allocationDesc, counterBufferAllocation);

		if (counterBufferAllocation.bufferResource == nullptr)
			throw std::exception("RWBufferFactory::CreateResource: Counter Resource is null!");
	}

	tempDescriptorAllocator->Allocate(_device, descriptorAllocationDesc, unorderedAccessDescriptorAllocation);
	_device->CreateUnorderedAccessView(bufferAllocation.bufferResource, counterBufferAllocation.bufferResource,
		&unorderedAccessViewDesc, unorderedAccessDescriptorAllocation.descriptorBase);

	Memory::ResourceData resourceData{};
	resourceData.shaderResourceViewDesc = std::make_unique<D3D12_SHADER_RESOURCE_VIEW_DESC>(std::move(shaderResourceViewDesc));
	resourceData.unorderedAccessViewDesc = std::make_unique<D3D12_UNORDERED_ACCESS_VIEW_DESC>(std::move(unorderedAccessViewDesc));
	resourceData.currentResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	resourceData.descriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(descriptorAllocation));
	resourceData.shaderNonVisibleDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(shaderNonVisibleDescriptorAllocation));
	resourceData.unorderedDescriptorAllocation = std::make_unique<DescriptorAllocation>(std::move(unorderedAccessDescriptorAllocation));
	resourceData.bufferAllocation = std::make_unique<BufferAllocation>(std::move(bufferAllocation));

	return resourceData;
}
