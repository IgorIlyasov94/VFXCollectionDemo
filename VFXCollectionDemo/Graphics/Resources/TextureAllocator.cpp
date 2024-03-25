#include "TextureAllocator.h"

void Memory::TextureAllocator::Allocate(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_CLEAR_VALUE* clearValue,
    const Graphics::TextureInfo& textureInfo, TextureAllocation& allocation)
{
    Memory::TextureAllocationPage::PageDesc desc{};
    desc.clearValue = clearValue;
    desc.heapType = D3D12_HEAP_TYPE_DEFAULT;
    desc.resourceFlags = resourceFlags;

    pages.push_back(std::make_shared<TextureAllocationPage>(device, desc, textureInfo));

    pages[pages.size() - 1]->GetAllocation(allocation);
}

void Memory::TextureAllocator::AllocateTemporaryUpload(ID3D12Device* device, D3D12_RESOURCE_FLAGS resourceFlags,
    const Graphics::TextureInfo& textureInfo, TextureAllocation& allocation)
{
    Memory::TextureAllocationPage::PageDesc desc{};
    desc.clearValue = nullptr;
    desc.heapType = D3D12_HEAP_TYPE_UPLOAD;
    desc.resourceFlags = resourceFlags;

    tempUploadPages.push_back(std::make_shared<TextureAllocationPage>(device, desc, textureInfo));

    tempUploadPages[tempUploadPages.size() - 1]->GetAllocation(allocation);
}

void Memory::TextureAllocator::ReleaseTemporaryBuffers()
{
    tempUploadPages.clear();
}