#include "DescriptorAllocator.h"

void Memory::DescriptorAllocator::Allocate(ID3D12Device* device, AllocationDesc& desc, DescriptorAllocation& allocation)
{
    if (desc.isUAVShaderNonVisible)
        Allocate(device, desc, usedUavShaderNonVisiblePages, emptyUavShaderNonVisiblePages,
            currentUavShaderNonVisiblePage, allocation);
    else if (desc.descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        Allocate(device, desc, usedCbvSrvUavPages, emptyCbvSrvUavPages, currentCbvSrvUavPage, allocation);
    else if (desc.descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        Allocate(device, desc, usedSamplerPages, emptySamplerPages, currentSamplerPage, allocation);
    else if (desc.descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        Allocate(device, desc, usedRtvPages, emptyRtvPages, currentRtvPage, allocation);
    else if (desc.descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        Allocate(device, desc, usedDsvPages, emptyDsvPages, currentDsvPage, allocation);
}

ID3D12DescriptorHeap* Memory::DescriptorAllocator::GetCurrentCbvSrvUavDescriptorHeap() const noexcept
{
    if (currentCbvSrvUavPage != nullptr)
        return currentCbvSrvUavPage->GetDescriptorHeap();

    return nullptr;
}

ID3D12DescriptorHeap* Memory::DescriptorAllocator::GetCurrentSamplerDescriptorHeap() const noexcept
{
    if (currentSamplerPage != nullptr)
        return currentSamplerPage->GetDescriptorHeap();

    return nullptr;
}

void Memory::DescriptorAllocator::Allocate(ID3D12Device* device, AllocationDesc& desc, DescriptorHeapPool& usedHeapPool,
    DescriptorHeapPool& emptyHeapPool, std::shared_ptr<DescriptorAllocationPage>& currentPage, DescriptorAllocation& allocation)
{
    if (currentPage.get() == nullptr || !currentPage->HasSpace(desc.numDescriptors))
        SetNewPageAsCurrent(device, desc, usedHeapPool, emptyHeapPool, currentPage);

    currentPage->Allocate(desc.numDescriptors, allocation);
}

void Memory::DescriptorAllocator::SetNewPageAsCurrent(ID3D12Device* device, AllocationDesc& desc,
    DescriptorHeapPool& usedHeapPool, DescriptorHeapPool& emptyHeapPool, std::shared_ptr<DescriptorAllocationPage>& currentPage)
{
    if (emptyHeapPool.empty())
    {
        auto newPage = new DescriptorAllocationPage(device, desc.descriptorType, desc.isUAVShaderNonVisible, numDescriptorsPerHeap);
        currentPage = std::shared_ptr<DescriptorAllocationPage>(std::move(newPage));

        usedHeapPool.push_back(currentPage);
    }
    else
    {
        currentPage = emptyHeapPool.front();

        emptyHeapPool.pop_front();
    }
}
