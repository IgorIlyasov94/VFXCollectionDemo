#include "DescriptorAllocationPage.h"

Memory::DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& source) noexcept
	: descriptorIncrementSize(std::exchange(source.descriptorIncrementSize, 0U)),
	descriptorBase(std::move(source.descriptorBase)),
	gpuDescriptorBase(std::move(source.gpuDescriptorBase)),
	descriptorHeap(std::exchange(source.descriptorHeap, nullptr))
{

}

Memory::DescriptorAllocationPage::DescriptorAllocationPage(ID3D12Device* device,
	D3D12_DESCRIPTOR_HEAP_TYPE _descriptorHeapType, bool isUAVShaderNonVisible, uint32_t _numDescriptors)
	: numDescriptors(_numDescriptors), descriptorHeapType(_descriptorHeapType), descriptorBaseOffset(0u),
	gpuDescriptorBaseOffset(0u)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Type = descriptorHeapType;

	if (descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV ||
		descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
		descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV &&
		isUAVShaderNonVisible)
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	else
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)),
		"DescriptorAllocationPage: Descriptor Heap creating error");

	numFreeHandles = numDescriptors;
	descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

	descriptorBaseOffset = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	gpuDescriptorBaseOffset = descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
}

void Memory::DescriptorAllocationPage::Allocate(uint32_t _numDescriptors, DescriptorAllocation& allocation)
{
	if (numFreeHandles < _numDescriptors)
		throw std::exception("DescriptorAllocationPage::Allocate: Bad allocation");

	allocation.descriptorBase.ptr = descriptorBaseOffset;
	allocation.gpuDescriptorBase.ptr = gpuDescriptorBaseOffset;
	allocation.descriptorIncrementSize = descriptorIncrementSize;
	allocation.descriptorHeap = descriptorHeap.Get();

	descriptorBaseOffset += static_cast<SIZE_T>(static_cast<uint64_t>(_numDescriptors) *
		static_cast<uint64_t>(descriptorIncrementSize));
	gpuDescriptorBaseOffset += static_cast<uint64_t>(static_cast<uint64_t>(_numDescriptors) *
		static_cast<uint64_t>(descriptorIncrementSize));
	numFreeHandles -= _numDescriptors;
}

bool Memory::DescriptorAllocationPage::HasSpace(uint32_t _numDescriptors) const noexcept
{
	return numFreeHandles > _numDescriptors;
}

ID3D12DescriptorHeap* Memory::DescriptorAllocationPage::GetDescriptorHeap() const noexcept
{
	return descriptorHeap.Get();
}
