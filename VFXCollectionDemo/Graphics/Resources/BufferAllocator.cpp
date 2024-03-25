#include "BufferAllocator.h"

void Memory::BufferAllocator::Allocate(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation)
{
	if (desc.size > pageSize)
		throw std::exception("BufferAllocator::Allocate: Bad allocation");

	if (desc.heapType == D3D12_HEAP_TYPE_DEFAULT)
		Allocate(device, desc, false, false, emptyDefaultPages, usedDefaultPages, currentDefaultPage, allocation);
	else if (desc.heapType == D3D12_HEAP_TYPE_UPLOAD)
		Allocate(device, desc, false, false, emptyUploadPages, usedUploadPages, currentUploadPage, allocation);
}

void Memory::BufferAllocator::AllocateCustomBuffer(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation)
{
	if (desc.size > pageSize)
		throw std::exception("BufferAllocator::AllocateCustomBuffer: Bad allocation");

	desc.heapType = D3D12_HEAP_TYPE_DEFAULT;

	Allocate(device, desc, false, true, emptyCustomPages, usedCustomPages, currentCustomPage, allocation);
}

void Memory::BufferAllocator::AllocateUnorderedAccess(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation)
{
	if (desc.size > pageSize)
		throw std::exception("BufferAllocator::AllocateUnorderedAccess: Bad allocation");

	desc.heapType = D3D12_HEAP_TYPE_DEFAULT;

	Allocate(device, desc, true, true, emptyUnorderedPages, usedUnorderedPages, currentUnorderedPage, allocation);
}

void Memory::BufferAllocator::AllocateTemporary(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation)
{
	auto newPage = new BufferAllocationPage(device, desc.heapType, D3D12_RESOURCE_FLAG_NONE, desc.size);

	tempUploadPages.push_back(std::shared_ptr<BufferAllocationPage>(newPage));

	tempUploadPages.back()->Allocate(desc.size, 1, allocation);
}

void Memory::BufferAllocator::ReleaseTemporaryBuffers()
{
	tempUploadPages.clear();
}

void Memory::BufferAllocator::Allocate(ID3D12Device* device, AllocationDesc& desc, bool unorderedAccess, bool isUniqueBuffer,
	BufferAllocationPagePool& emptyPagePool, BufferAllocationPagePool& usedPagePool,
	std::shared_ptr<BufferAllocationPage>& currentPage, BufferAllocation& allocation)
{
	if (!currentPage || !currentPage->HasSpace(desc.size, desc.alignment) || isUniqueBuffer)
		SetNewPageAsCurrent(device, desc.heapType, unorderedAccess, emptyPagePool, usedPagePool, currentPage);

	currentPage->Allocate(desc.size, desc.alignment, allocation);
}

void Memory::BufferAllocator::SetNewPageAsCurrent(ID3D12Device* device, D3D12_HEAP_TYPE heapType,
	bool unorderedAccess, BufferAllocationPagePool& emptyPagePool,
	BufferAllocationPagePool& usedPagePool, std::shared_ptr<BufferAllocationPage>& currentPage)
{
	if (emptyPagePool.empty())
	{
		currentPage = std::shared_ptr<BufferAllocationPage>(new BufferAllocationPage(device, heapType,
			(unorderedAccess) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE, pageSize));

		usedPagePool.push_back(currentPage);
	}
	else
	{
		currentPage = emptyPagePool.front();

		emptyPagePool.pop_front();
	}
}
