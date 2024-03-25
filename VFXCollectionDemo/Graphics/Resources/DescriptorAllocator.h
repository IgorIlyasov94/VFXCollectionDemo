#pragma once

#include "..\..\Includes.h"
#include "DescriptorAllocationPage.h"

namespace Memory
{
	class DescriptorAllocator
	{
	public:
		using AllocationDesc = struct
		{
			uint32_t numDescriptors;
			D3D12_DESCRIPTOR_HEAP_TYPE descriptorType;
			bool isUAVShaderNonVisible;
		};

		DescriptorAllocator() {};
		~DescriptorAllocator() {};

		void Allocate(ID3D12Device* device, AllocationDesc& desc, DescriptorAllocation& allocation);
		ID3D12DescriptorHeap* GetCurrentCbvSrvUavDescriptorHeap() const noexcept;
		ID3D12DescriptorHeap* GetCurrentSamplerDescriptorHeap() const noexcept;

	private:
		using DescriptorHeapPool = std::deque<std::shared_ptr<DescriptorAllocationPage>>;

		void Allocate(ID3D12Device* device, AllocationDesc& desc, DescriptorHeapPool& usedHeapPool,
			DescriptorHeapPool& emptyHeapPool, std::shared_ptr<DescriptorAllocationPage>& currentPage,
			DescriptorAllocation& allocation);

		void SetNewPageAsCurrent(ID3D12Device* device, AllocationDesc& desc, DescriptorHeapPool& usedHeapPool,
			DescriptorHeapPool& emptyHeapPool, std::shared_ptr<DescriptorAllocationPage>& currentPage);

		DescriptorHeapPool usedCbvSrvUavPages;
		DescriptorHeapPool usedSamplerPages;
		DescriptorHeapPool usedRtvPages;
		DescriptorHeapPool usedDsvPages;
		DescriptorHeapPool usedUavShaderNonVisiblePages;

		DescriptorHeapPool emptyCbvSrvUavPages;
		DescriptorHeapPool emptySamplerPages;
		DescriptorHeapPool emptyRtvPages;
		DescriptorHeapPool emptyDsvPages;
		DescriptorHeapPool emptyUavShaderNonVisiblePages;

		std::shared_ptr<DescriptorAllocationPage> currentCbvSrvUavPage;
		std::shared_ptr<DescriptorAllocationPage> currentSamplerPage;
		std::shared_ptr<DescriptorAllocationPage> currentRtvPage;
		std::shared_ptr<DescriptorAllocationPage> currentDsvPage;
		std::shared_ptr<DescriptorAllocationPage> currentUavShaderNonVisiblePage;

		const uint32_t numDescriptorsPerHeap = 4096;
	};
}
