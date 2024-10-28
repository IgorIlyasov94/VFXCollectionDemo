#include "RenderTargetFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::RenderTargetFactory::RenderTargetFactory(TextureManager* textureManager, DescriptorManager* descriptorManager)
	: _textureManager(textureManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::RenderTargetFactory::~RenderTargetFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::RenderTargetFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& textureDesc = static_cast<const TextureDesc*>(desc)[0];
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = textureDesc.format;
	
	auto textureAllocation = _textureManager->Allocate(device, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, clearValue, textureDesc);

	auto renderTarget = new RenderTarget;
	std::swap(renderTarget->resource, textureAllocation.resource);
	renderTarget->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	renderTarget->rtvDescriptor = _descriptorManager->Allocate(DescriptorType::RTV);
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(textureDesc);
	auto rtvDesc = DirectX12Utilities::CreateRTVDesc(textureDesc);
	
	auto resource = renderTarget->resource->GetResource();

	device->CreateShaderResourceView(resource, &srvDesc, renderTarget->srvDescriptor.cpuDescriptor);
	device->CreateRenderTargetView(resource, &rtvDesc, renderTarget->rtvDescriptor.cpuDescriptor);

	return static_cast<IResource*>(renderTarget);
}
