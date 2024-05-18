#include "DescriptorManager.h"

Graphics::DescriptorManager::DescriptorManager(ID3D12Device* device)
{
	ID3D12DescriptorHeap* heap = nullptr;

	CreateDescriptor(device, MAX_CBV_SRV_UAV_DESCRIPTORS_NUMBER, DescriptorType::CBV_SRV_UAV);
	CreateDescriptor(device, MAX_SAMPLER_DESCRIPTORS_NUMBER, DescriptorType::SAMPLER);
	CreateDescriptor(device, MAX_RTV_DESCRIPTORS_NUMBER, DescriptorType::RTV);
	CreateDescriptor(device, MAX_DSV_DESCRIPTORS_NUMBER, DescriptorType::DSV);
	CreateDescriptor(device, MAX_NON_SHADER_VISIBLE_DESCRIPTORS_NUMBER, DescriptorType::CBV_SRV_UAV_NON_SHADER_VISIBLE);
}

Graphics::DescriptorManager::~DescriptorManager()
{
	for (auto& descriptor : descriptors)
		descriptor.second.heap->Release();
}

Graphics::DescriptorAllocation Graphics::DescriptorManager::Allocate(DescriptorType type)
{
	auto& descriptor = descriptors[type];

	SIZE_T cpuPtr{};
	uint64_t gpuPtr{};

	if (descriptor.freeSpace.empty())
	{
		cpuPtr = descriptor.offset.base;
		gpuPtr = descriptor.offset.gpuBase;

		descriptor.offset.base += descriptor.incrementSize;
		descriptor.offset.gpuBase += descriptor.incrementSize;
	}
	else
	{
		auto& offset = descriptor.freeSpace.front();

		cpuPtr = offset.base;
		gpuPtr = offset.gpuBase;

		descriptor.freeSpace.pop();
	}

	DescriptorAllocation allocation{};
	allocation.cpuDescriptor.ptr = cpuPtr;
	allocation.gpuDescriptor.ptr = gpuPtr;

	return allocation;
}

void Graphics::DescriptorManager::Deallocate(DescriptorType type, const DescriptorAllocation& allocation)
{
	auto& descriptor = descriptors[type];

	DescriptorOffset offset{};
	offset.base = allocation.cpuDescriptor.ptr;
	offset.gpuBase = allocation.gpuDescriptor.ptr;

	descriptor.freeSpace.push(std::move(offset));
}

ID3D12DescriptorHeap* Graphics::DescriptorManager::GetHeap(DescriptorType type)
{
	return descriptors[type].heap;
}

void Graphics::DescriptorManager::CreateDescriptor(ID3D12Device* device, uint32_t number, DescriptorType type)
{
	ID3D12DescriptorHeap* heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = ConvertType(type);
	desc.NumDescriptors = number;
	desc.Flags = (type == DescriptorType::CBV_SRV_UAV || type == DescriptorType::SAMPLER) ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));

	Descriptor descriptor{};
	descriptor.heap = heap;
	descriptor.offset.base = heap->GetCPUDescriptorHandleForHeapStart().ptr;
	descriptor.offset.gpuBase = heap->GetGPUDescriptorHandleForHeapStart().ptr;
	descriptor.incrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);

	descriptors.insert({ type , descriptor });
}

D3D12_DESCRIPTOR_HEAP_TYPE Graphics::DescriptorManager::ConvertType(DescriptorType type)
{
	if (type == DescriptorType::DSV)
		return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	else if (type == DescriptorType::SAMPLER)
		return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	else if (type == DescriptorType::RTV)
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	else
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}
