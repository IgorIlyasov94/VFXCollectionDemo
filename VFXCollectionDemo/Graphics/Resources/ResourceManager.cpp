#include "ResourceManager.h"
#include "BufferFactory.h"
#include "ConstantBufferFactory.h"
#include "DepthStencilFactory.h"
#include "IndexBufferFactory.h"
#include "RenderTargetFactory.h"
#include "RWBufferFactory.h"
#include "RWTextureFactory.h"
#include "TextureFactory.h"
#include "VertexBufferFactory.h"

Graphics::Resources::ResourceManager::ResourceManager(DescriptorManager* descriptorManager, BufferManager* bufferManager,
	TextureManager* textureManager)
	: _bufferManager(bufferManager), _textureManager(textureManager), _descriptorManager(descriptorManager)
{
	bufferFactories[EnumValue(BufferResourceType::BUFFER)] = new BufferFactory(_bufferManager, _descriptorManager);
	bufferFactories[EnumValue(BufferResourceType::CONSTANT_BUFFER)] = new ConstantBufferFactory(_bufferManager, _descriptorManager);
	bufferFactories[EnumValue(BufferResourceType::INDEX_BUFFER)] = new IndexBufferFactory(_bufferManager);
	bufferFactories[EnumValue(BufferResourceType::RW_BUFFER)] = new RWBufferFactory(_bufferManager, _descriptorManager);
	bufferFactories[EnumValue(BufferResourceType::VERTEX_BUFFER)] = new VertexBufferFactory(_bufferManager);

	textureFactories[EnumValue(TextureResourceType::DEPTH_STENCIL_TARGET)] = new DepthStencilFactory(textureManager, _descriptorManager);
	textureFactories[EnumValue(TextureResourceType::RENDER_TARGET)] = new RenderTargetFactory(textureManager, _descriptorManager);
	textureFactories[EnumValue(TextureResourceType::RW_TEXTURE)] = new RWTextureFactory(textureManager, _descriptorManager);
	textureFactories[EnumValue(TextureResourceType::TEXTURE)] = new TextureFactory(textureManager, _descriptorManager);
}

Graphics::Resources::ResourceManager::~ResourceManager()
{
	for (auto& resource : resources)
		if (resource != nullptr)
			delete resource;
}

Graphics::Resources::ResourceID Graphics::Resources::ResourceManager::CreateBufferResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, BufferResourceType type, const BufferDesc& desc)
{
	auto resourceDesc = static_cast<const IResourceDesc*>(&desc);
	auto resource = bufferFactories[EnumValue(type)]->CreateResource(device, commandList, resourceDesc);

	auto id = static_cast<ResourceID>(resources.size());

	if (freeSlots.empty())
		resources.push_back(std::move(resource));
	else
	{
		id = freeSlots.front();
		freeSlots.pop();

		resources[id] = resource;
	}

	return id;
}

Graphics::Resources::ResourceID Graphics::Resources::ResourceManager::CreateTextureResource(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, TextureResourceType type, const TextureDesc& desc)
{
	auto resourceDesc = static_cast<const IResourceDesc*>(&desc);
	auto resource = textureFactories[EnumValue(type)]->CreateResource(device, commandList, resourceDesc);
	
	auto id = static_cast<ResourceID>(resources.size());

	if (freeSlots.empty())
		resources.push_back(std::move(resource));
	else
	{
		id = freeSlots.front();
		freeSlots.pop();

		resources[id] = resource;
	}

	return id;
}

Graphics::Resources::ResourceID Graphics::Resources::ResourceManager::CreateSamplerResource(ID3D12Device* device,
	D3D12_SAMPLER_DESC desc)
{
	auto newSampler = new Sampler;
	newSampler->samplerDescriptor = _descriptorManager->Allocate(DescriptorType::SAMPLER);
	newSampler->samplerDesc = desc;

	device->CreateSampler(&desc, newSampler->samplerDescriptor.cpuDescriptor);

	auto id = static_cast<ResourceID>(resources.size());

	if (freeSlots.empty())
		resources.push_back(std::move(newSampler));
	else
	{
		id = freeSlots.front();
		freeSlots.pop();

		resources[id] = newSampler;
	}

	return id;
}
