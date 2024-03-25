#include "BufferAllocationPage.h"
#include "..\DirectX12Helper.h"

Memory::BufferAllocation::BufferAllocation(BufferAllocation&& source) noexcept
	: cpuAddress(std::exchange(source.cpuAddress, nullptr)),
	gpuAddress(std::exchange(source.gpuAddress, 0U)),
	gpuPageOffset(std::exchange(source.gpuPageOffset, 0U)),
	nonAlignedSizeInBytes(std::exchange(source.nonAlignedSizeInBytes, 0U)),
	bufferResource(std::exchange(source.bufferResource, nullptr))
{

}

Memory::BufferAllocationPage::BufferAllocationPage(ID3D12Device* device, D3D12_HEAP_TYPE _heapType,
	D3D12_RESOURCE_FLAGS resourceFlags, uint64_t _pageSize)
	: heapType(_heapType), pageSize(_pageSize), offset(0), currentCPUAddress(nullptr),
	currentGPUAddress(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	D3D12_HEAP_PROPERTIES heapProperties;
	Graphics::DirectX12Helper::SetupHeapProperties(heapProperties, heapType);

	D3D12_RESOURCE_DESC resourceDesc;
	Graphics::DirectX12Helper::SetupResourceBufferDesc(resourceDesc, _pageSize, resourceFlags);

	D3D12_RESOURCE_STATES resourceState;

	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	else if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		resourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	else
		resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, resourceState, nullptr,
		IID_PPV_ARGS(&pageResource)), "BufferAllocationPage::BufferAllocationPage: Resource creating error");

	D3D12_RANGE range = { 0, 0 };

	if (heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_READBACK)
		ThrowIfFailed(pageResource->Map(0, &range, reinterpret_cast<void**>(&currentCPUAddress)),
			"BufferAllocationPage::BufferAllocationPage: Resource mapping error");

	currentGPUAddress = pageResource->GetGPUVirtualAddress();
}

Memory::BufferAllocationPage::~BufferAllocationPage()
{
	if (heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_READBACK)
		pageResource->Unmap(0, nullptr);

	currentCPUAddress = nullptr;
	currentGPUAddress = D3D12_GPU_VIRTUAL_ADDRESS(0);

	offset = 0;
}

void Memory::BufferAllocationPage::Allocate(uint64_t _size, uint64_t alignment, BufferAllocation& allocation)
{
	if (!HasSpace(_size, alignment))
		throw std::exception("BufferAllocationPage::Allocate: Bad allocation");

	auto alignedSize = Common::CommonUtility::AlignSize(_size, alignment);
	offset = Common::CommonUtility::AlignSize(offset, alignment);

	allocation.cpuAddress = currentCPUAddress + offset;
	allocation.gpuAddress = currentGPUAddress + offset;
	allocation.gpuPageOffset = offset;
	allocation.nonAlignedSizeInBytes = _size;
	allocation.bufferResource = pageResource.Get();

	offset += alignedSize;
}

bool Memory::BufferAllocationPage::HasSpace(uint64_t _size, uint64_t alignment) const noexcept
{
	auto alignedSize = Common::CommonUtility::AlignSize(_size, alignment);
	auto alignedOffset = Common::CommonUtility::AlignSize(offset, alignment);

	return alignedSize + alignedOffset <= pageSize;
}
