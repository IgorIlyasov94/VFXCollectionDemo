#pragma once

#include "..\..\Includes.h"

namespace Memory
{
	struct DescriptorAllocation
	{
	public:
		DescriptorAllocation() = default;
		~DescriptorAllocation() {};
		DescriptorAllocation(DescriptorAllocation&& source) noexcept;

		uint32_t descriptorIncrementSize;
		D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorBase;
		ID3D12DescriptorHeap* descriptorHeap;
	};

	class DescriptorAllocationPage
	{
	public:
		DescriptorAllocationPage(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE _descriptorHeapType,
			bool isUAVShaderNonVisible, uint32_t _numDescriptors);
		~DescriptorAllocationPage() {};

		void Allocate(uint32_t _numDescriptors, DescriptorAllocation& allocation);
		bool HasSpace(uint32_t _numDescriptors) const noexcept;

		ID3D12DescriptorHeap* GetDescriptorHeap() const noexcept;

	private:
		uint32_t numDescriptors;
		uint32_t numFreeHandles;
		uint32_t descriptorIncrementSize;

		ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		SIZE_T descriptorBaseOffset;
		uint64_t gpuDescriptorBaseOffset;
		D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType;
	};
}
