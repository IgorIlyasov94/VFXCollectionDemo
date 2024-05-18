#include "DepthStencilFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::DepthStencilFactory::DepthStencilFactory(TextureManager* textureManager, DescriptorManager* descriptorManager)
	: _textureManager(textureManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::DepthStencilFactory::~DepthStencilFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::DepthStencilFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& textureDesc = static_cast<const TextureDesc*>(desc)[0];
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = textureDesc.format;

	auto textureAllocation = _textureManager->Allocate(device, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, clearValue, textureDesc);

	auto depthStencilTarget = new DepthStencilTarget;
	std::swap(depthStencilTarget->resource, textureAllocation.resource);
	depthStencilTarget->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	depthStencilTarget->dsvDescriptor = _descriptorManager->Allocate(DescriptorType::DSV);
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(textureDesc);
	auto dsvDesc = DirectX12Utilities::CreateDSVDesc(textureDesc);

	auto resource = depthStencilTarget->resource->GetResource();

	device->CreateShaderResourceView(resource, &srvDesc, depthStencilTarget->srvDescriptor.cpuDescriptor);
	device->CreateDepthStencilView(resource, &dsvDesc, depthStencilTarget->dsvDescriptor.cpuDescriptor);

	return static_cast<IResource*>(depthStencilTarget);
}
