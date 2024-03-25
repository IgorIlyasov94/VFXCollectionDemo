#pragma once

#include "BufferAllocationPage.h"
#include "..\DirectX12Helper.h"

namespace Memory
{
	class BufferAllocator
	{
	public:
		using AllocationDesc = struct
		{
			size_t size;
			size_t alignment;
			D3D12_HEAP_TYPE heapType;
		};

		BufferAllocator() {};
		~BufferAllocator() {};

		void Allocate(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation);
		void AllocateCustomBuffer(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation);
		void AllocateUnorderedAccess(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation);
		void AllocateTemporary(ID3D12Device* device, AllocationDesc& desc, BufferAllocation& allocation);

		void ReleaseTemporaryBuffers();

	private:
		using BufferAllocationPagePool = std::deque<std::shared_ptr<BufferAllocationPage>>;

		void Allocate(ID3D12Device* device, AllocationDesc& desc, bool unorderedAccess, bool isUniqueBuffer,
			BufferAllocationPagePool& emptyPagePool, BufferAllocationPagePool& usedPagePool,
			std::shared_ptr<BufferAllocationPage>& currentPage, BufferAllocation& allocation);

		void SetNewPageAsCurrent(ID3D12Device* device, D3D12_HEAP_TYPE heapType, bool unorderedAccess,
			BufferAllocationPagePool& emptyPagePool, BufferAllocationPagePool& usedPagePool,
			std::shared_ptr<BufferAllocationPage>& currentPage);

		BufferAllocationPagePool usedDefaultPages;
		BufferAllocationPagePool usedUploadPages;
		BufferAllocationPagePool usedCustomPages;
		BufferAllocationPagePool usedUnorderedPages;

		BufferAllocationPagePool emptyDefaultPages;
		BufferAllocationPagePool emptyUploadPages;
		BufferAllocationPagePool emptyCustomPages;
		BufferAllocationPagePool emptyUnorderedPages;

		std::shared_ptr<BufferAllocationPage> currentDefaultPage;
		std::shared_ptr<BufferAllocationPage> currentUploadPage;
		std::shared_ptr<BufferAllocationPage> currentCustomPage;
		std::shared_ptr<BufferAllocationPage> currentUnorderedPage;

		BufferAllocationPagePool tempUploadPages;

		static constexpr size_t pageSize = 10 * Graphics::_MB;
	};
}
