#pragma once

#include "../DirectX12Includes.h"
#include "../DirectX12Utilities.h"
#include "../CommandManager.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "../TextureManager.h"
#include "../Assets/Loaders/HLSLLoader.h"
#include "IResource.h"
#include "IResourceDesc.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	template<typename T>
	concept ResourceType = std::derived_from<T, IResource>;

	using ResourceID = uint32_t;

	enum class BufferResourceType : uint32_t
	{
		BUFFER = 0,
		CONSTANT_BUFFER = 1,
		INDEX_BUFFER = 2,
		RW_BUFFER = 3,
		VERTEX_BUFFER = 4
	};

	enum class TextureResourceType : uint32_t
	{
		DEPTH_STENCIL_TARGET = 0,
		RENDER_TARGET = 1,
		RW_TEXTURE = 2,
		TEXTURE = 3
	};

	class ResourceManager
	{
	public:
		ResourceManager(DescriptorManager* descriptorManager, BufferManager* bufferManager, TextureManager* textureManager);
		~ResourceManager();

		ResourceID CreateBufferResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			BufferResourceType type, const BufferDesc& desc);

		ResourceID CreateTextureResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			TextureResourceType type, const TextureDesc& desc);

		ResourceID CreateSamplerResource(ID3D12Device* device, D3D12_SAMPLER_DESC desc);

		ResourceID CreateShaderResource(ID3D12Device* device, std::filesystem::path filePath,
			Assets::Loaders::ShaderType type, Assets::Loaders::ShaderVersion version);

		Sampler* GetDefaultSampler(ID3D12Device* device, Graphics::DefaultFilterSetup filter);

		template<ResourceType T>
		T* GetResource(ResourceID id)
		{
			return static_cast<T*>(resources[id]);
		}

		template<ResourceType T>
		void DeleteResource(ResourceID id)
		{
			auto resource = static_cast<T*>(resources[id]);

			if constexpr (std::is_same_v<T, Buffer> || std::is_same_v<T, DepthStencilTarget> ||
				std::is_same_v<T, RenderTarget> || std::is_same_v<T, RWBuffer> ||
				std::is_same_v<T, RWTexture> || std::is_same_v<T, Texture>)
				_descriptorManager->Deallocate(DescriptorType::CBV_SRV_UAV, resource->srvDescriptor);

			if constexpr (std::is_same_v<T, ConstantBuffer>)
				_descriptorManager->Deallocate(DescriptorType::CBV_SRV_UAV, resource->cbvDescriptor);

			if constexpr (std::is_same_v<T, DepthStencilTarget>)
				_descriptorManager->Deallocate(DescriptorType::DSV, resource->dsvDescriptor);

			if constexpr (std::is_same_v<T, RenderTarget>)
				_descriptorManager->Deallocate(DescriptorType::RTV, resource->rtvDescriptor);

			if constexpr (std::is_same_v<T, RWBuffer> || std::is_same_v<T, RWTexture>)
			{
				_descriptorManager->Deallocate(DescriptorType::CBV_SRV_UAV, resource->uavDescriptor);
				_descriptorManager->Deallocate(DescriptorType::CBV_SRV_UAV_NON_SHADER_VISIBLE, resource->uavNonShaderVisibleDescriptor);
			}

			if constexpr (std::is_same_v<T, Buffer> || std::is_same_v<T, ConstantBuffer> || std::is_same_v<T, RWBuffer>)
				_bufferManager->Deallocate(resource->resource, resource->resourceGPUAddress, resource->size);

			if constexpr (std::is_same_v<T, VertexBuffer> || std::is_same_v<T, IndexBuffer>)
				_bufferManager->Deallocate(resource->resource, resource->viewDesc.BufferLocation, resource->viewDesc.SizeInBytes);

			if constexpr (std::is_same_v<T, DepthStencilTarget> || std::is_same_v<T, RenderTarget> ||
				std::is_same_v<T, RWTexture> || std::is_same_v<T, Texture>)
				_textureManager->Deallocate(resource->resource);

			delete resources[id];
			resources[id] = nullptr;

			freeSlots.push(id);
		}

	private:
		ResourceManager() = delete;
		ResourceManager(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		template<typename T>
		constexpr std::underlying_type_t<T> EnumValue(T value)
		{
			return static_cast<std::underlying_type_t<T>>(value);
		}

		static constexpr size_t BUFFER_RESOURCE_TYPES_NUMBER = 5u;
		static constexpr size_t TEXTURE_RESOURCE_TYPES_NUMBER = 4u;

		std::vector<IResource*> resources;
		std::queue<ResourceID> freeSlots;

		std::map<Graphics::DefaultFilterSetup, ResourceID> defaultSamplers;

		std::array<IResourceFactory*, BUFFER_RESOURCE_TYPES_NUMBER> bufferFactories;
		std::array<IResourceFactory*, TEXTURE_RESOURCE_TYPES_NUMBER> textureFactories;

		BufferManager* _bufferManager;
		TextureManager* _textureManager;
		DescriptorManager* _descriptorManager;
	};
}
