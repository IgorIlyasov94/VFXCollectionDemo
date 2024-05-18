#pragma once

#include "DirectX12Includes.h"

namespace Graphics
{
	enum class DescriptorType : uint32_t
	{
		CBV_SRV_UAV = 0,
		SAMPLER = 1,
		RTV = 2,
		DSV = 3,
		CBV_SRV_UAV_NON_SHADER_VISIBLE = 4
	};

	struct DescriptorAllocation
	{
	public:
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor;
	};

	class DescriptorManager
	{
	public:
		DescriptorManager(ID3D12Device* device);
		~DescriptorManager();

		DescriptorAllocation Allocate(DescriptorType type);
		void Deallocate(DescriptorType type, const DescriptorAllocation& allocation);

		ID3D12DescriptorHeap* GetHeap(DescriptorType type);

	private:
		DescriptorManager() = delete;
		DescriptorManager(const DescriptorManager&) = delete;
		DescriptorManager(DescriptorManager&&) = delete;
		DescriptorManager& operator=(const DescriptorManager&) = delete;
		DescriptorManager& operator=(DescriptorManager&&) = delete;

		void CreateDescriptor(ID3D12Device* device, uint32_t number, DescriptorType type);
		D3D12_DESCRIPTOR_HEAP_TYPE ConvertType(DescriptorType type);

		static const uint32_t MAX_CBV_SRV_UAV_DESCRIPTORS_NUMBER = 512;
		static const uint32_t MAX_SAMPLER_DESCRIPTORS_NUMBER = 32;
		static const uint32_t MAX_RTV_DESCRIPTORS_NUMBER = 64;
		static const uint32_t MAX_DSV_DESCRIPTORS_NUMBER = 64;
		static const uint32_t MAX_NON_SHADER_VISIBLE_DESCRIPTORS_NUMBER = 512;

		struct DescriptorOffset
		{
		public:
			DescriptorOffset()
				: base{}, gpuBase{}
			{

			}

			SIZE_T base;
			uint64_t gpuBase;
		};

		struct Descriptor
		{
		public:
			Descriptor()
				: heap{}, incrementSize{}
			{

			}

			ID3D12DescriptorHeap* heap;
			DescriptorOffset offset;
			uint32_t incrementSize;
			std::queue<DescriptorOffset> freeSpace;
		};

		std::map<DescriptorType, Descriptor> descriptors;
	};
}
