#include "RWTextureFactory.h"
#include "../DirectX12Utilities.h"

Graphics::Resources::RWTextureFactory::RWTextureFactory(TextureManager* textureManager, DescriptorManager* descriptorManager)
	: _textureManager(textureManager), _descriptorManager(descriptorManager)
{

}

Graphics::Resources::RWTextureFactory::~RWTextureFactory()
{

}

Graphics::Resources::IResource* Graphics::Resources::RWTextureFactory::CreateResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc)
{
	auto& textureDesc = static_cast<const TextureDesc*>(desc)[0];
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = textureDesc.format;

	auto textureAllocation = _textureManager->Allocate(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, clearValue, textureDesc);
	
	auto rwTexture = new RWTexture;
	std::swap(rwTexture->resource, textureAllocation.resource);
	rwTexture->srvDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	rwTexture->uavDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV);
	rwTexture->uavNonShaderVisibleDescriptor = _descriptorManager->Allocate(DescriptorType::CBV_SRV_UAV_NON_SHADER_VISIBLE);
	auto srvDesc = DirectX12Utilities::CreateSRVDesc(textureDesc);
	auto uavDesc = DirectX12Utilities::CreateUAVDesc(textureDesc);
	
	auto resource = rwTexture->resource->GetResource();

	device->CreateShaderResourceView(resource, &srvDesc, rwTexture->srvDescriptor.cpuDescriptor);
	device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, rwTexture->uavDescriptor.cpuDescriptor);
	device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, rwTexture->uavNonShaderVisibleDescriptor.cpuDescriptor);

	return static_cast<IResource*>(rwTexture);
}
