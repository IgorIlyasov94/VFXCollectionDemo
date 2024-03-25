#include "TextureAllocationPage.h"

Memory::TextureAllocation::TextureAllocation(TextureAllocation&& source) noexcept
	: cpuAddress(std::exchange(source.cpuAddress, nullptr)),
	textureResource(std::exchange(source.textureResource, nullptr))
{

}

Memory::TextureAllocationPage::TextureAllocationPage(ID3D12Device* device, PageDesc& desc, const Graphics::TextureInfo& textureInfo)
	: cpuAddress(nullptr), heapType(desc.heapType)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	Graphics::DirectX12Helper::SetupHeapProperties(heapProperties, heapType);

	D3D12_RESOURCE_DESC resourceDesc{};
	D3D12_RESOURCE_STATES resourceState;

	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		D3D12_RESOURCE_DESC destResourceDesc{};
		Graphics::DirectX12Helper::SetupResourceTextureDesc(destResourceDesc, textureInfo, desc.resourceFlags);

		uint64_t requiredUploadBufferSize;
		uint32_t numSubresources = textureInfo.depth * textureInfo.mipLevels;

		device->GetCopyableFootprints(&destResourceDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &requiredUploadBufferSize);

		Graphics::DirectX12Helper::SetupResourceBufferDesc(resourceDesc, requiredUploadBufferSize, desc.resourceFlags);
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else
	{
		Graphics::DirectX12Helper::SetupResourceTextureDesc(resourceDesc, textureInfo, desc.resourceFlags);
		resourceState = D3D12_RESOURCE_STATE_COMMON;
	}

	if (Common::CommonUtility::CheckEnum(desc.resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))
		resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	else if (Common::CommonUtility::CheckEnum(desc.resourceFlags, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
		resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, resourceState, desc.clearValue,
		IID_PPV_ARGS(&pageResource)), "TextureAllocationPage::TextureAllocationPage: Resource creating error");

	D3D12_RANGE range = { 0, 0 };

	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
		ThrowIfFailed(pageResource->Map(0, &range, reinterpret_cast<void**>(&cpuAddress)),
			"TextureAllocationPage::TextureAllocationPage: Resource mapping error");
}

Memory::TextureAllocationPage::~TextureAllocationPage()
{
	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
		pageResource->Unmap(0, nullptr);

	cpuAddress = nullptr;
}

void Memory::TextureAllocationPage::GetAllocation(TextureAllocation& allocation)
{
	allocation.cpuAddress = cpuAddress;
	allocation.textureResource = pageResource.Get();
}
