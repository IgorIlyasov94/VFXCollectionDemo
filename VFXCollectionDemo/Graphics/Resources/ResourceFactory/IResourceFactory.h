#pragma once

#include "..\..\..\Includes.h"
#include "..\BufferAllocator.h"
#include "..\DescriptorAllocator.h"
#include "..\TextureAllocator.h"
#include "..\..\DirectX12Helper.h"
#include "..\..\..\Common\CommonUtilities.h"

namespace Memory
{
	enum class ResourceType : uint8_t
	{
		VERTEX_BUFFER,
		INDEX_BUFFER,
		CONSTANT_BUFFER,
		TEXTURE,
		BUFFER,
		SAMPLER,
		RENDER_TARGET,
		DEPTH_STENCIL,
		RW_TEXTURE,
		RW_BUFFER,
		SWAP_CHAIN
	};

	inline ResourceType operator|(ResourceType leftValue, ResourceType rightValue)
	{
		using UnderlyingType = std::underlying_type_t<ResourceType>;

		return static_cast<ResourceType>(static_cast<UnderlyingType>(leftValue) | static_cast<UnderlyingType>(rightValue));
	}

	inline ResourceType operator&(ResourceType leftValue, ResourceType rightValue)
	{
		using UnderlyingType = std::underlying_type_t<ResourceType>;

		return static_cast<ResourceType>(static_cast<UnderlyingType>(leftValue) & static_cast<UnderlyingType>(rightValue));
	}

	class ResourceId
	{
	public:
		ResourceId()
			: id(0), type(static_cast<ResourceType>(0U))
		{};

		~ResourceId() {};

		ResourceId(size_t resourceId, ResourceType resourceType)
			: id(resourceId), type(resourceType)
		{

		}

		ResourceType GetType() const noexcept
		{
			return type;
		}

		size_t GetId() const noexcept
		{
			return id;
		}

	private:

		ResourceType type;
		size_t id;
	};

	struct ResourceDesc
	{
	public:
		ResourceDesc()
			: data(nullptr), preparedResource(nullptr), dataSize(0), dataStride(0), addCounter(false),
			isDynamic(false), numElements(0), textureInfo(nullptr)
		{

		}

		~ResourceDesc() {};

		void* data;
		ID3D12Resource* preparedResource;
		size_t dataSize;
		size_t dataStride;
		bool addCounter;
		bool isDynamic;

		union
		{
			size_t numElements;
			D3D12_RESOURCE_FLAGS resourceFlags;
			uint32_t depthBit;
		};

		union
		{
			std::shared_ptr<Graphics::TextureInfo> textureInfo;
			std::shared_ptr<D3D12_SAMPLER_DESC> samplerDesc;
			DXGI_FORMAT bufferFormat;
		};
	};

	struct ResourceData
	{
	public:
		ResourceData()
			: shaderResourceViewDesc(nullptr), currentResourceState(D3D12_RESOURCE_STATE_COMMON),
			descriptorAllocation(nullptr), shaderNonVisibleDescriptorAllocation(nullptr), indicesCount(0),
			textureInfo(nullptr), renderTargetDescriptorAllocation(nullptr), bufferAllocation(nullptr),
			vertexBufferView(nullptr)
		{};
		~ResourceData() {};

		ResourceData(ResourceData&& source) noexcept
			: shaderResourceViewDesc(std::move(source.shaderResourceViewDesc)),
			currentResourceState(std::exchange(source.currentResourceState, D3D12_RESOURCE_STATE_COMMON)),
			descriptorAllocation(std::move(source.descriptorAllocation)),
			shaderNonVisibleDescriptorAllocation(std::move(source.shaderNonVisibleDescriptorAllocation)),
			textureInfo(std::move(source.textureInfo)),
			indicesCount(std::exchange(source.indicesCount, 0U)),
			renderTargetDescriptorAllocation(std::move(source.renderTargetDescriptorAllocation)),
			bufferAllocation(std::move(source.bufferAllocation)),
			vertexBufferView(std::move(source.vertexBufferView))
		{

		}

		ResourceData& operator=(ResourceData&& source) noexcept
		{
			std::swap(shaderResourceViewDesc, source.shaderResourceViewDesc);
			std::swap(currentResourceState, source.currentResourceState);
			std::swap(descriptorAllocation, source.descriptorAllocation);
			std::swap(indicesCount, source.indicesCount);
			std::swap(bufferAllocation, source.bufferAllocation);
			std::swap(vertexBufferView, source.vertexBufferView);

			return *this;
		}

		void Release()
		{
			shaderResourceViewDesc.reset();
			currentResourceState = D3D12_RESOURCE_STATE_COMMON;
			descriptorAllocation.reset();
			shaderNonVisibleDescriptorAllocation.reset();
			textureInfo.reset();
			renderTargetDescriptorAllocation.reset();
			bufferAllocation.reset();
			vertexBufferView.reset();
		}

		std::unique_ptr<D3D12_SHADER_RESOURCE_VIEW_DESC> shaderResourceViewDesc;
		D3D12_RESOURCE_STATES currentResourceState;

		std::unique_ptr<DescriptorAllocation> descriptorAllocation;
		std::unique_ptr<DescriptorAllocation> shaderNonVisibleDescriptorAllocation;
		std::shared_ptr<Graphics::TextureInfo> textureInfo;
		uint32_t indicesCount;

		union
		{
			std::unique_ptr<DescriptorAllocation> renderTargetDescriptorAllocation;
			std::unique_ptr<DescriptorAllocation> depthStencilDescriptorAllocation;
			std::unique_ptr<DescriptorAllocation> unorderedDescriptorAllocation;
		};

		union
		{
			std::unique_ptr<BufferAllocation> bufferAllocation;
			std::unique_ptr<TextureAllocation> textureAllocation;
		};

		union
		{
			std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
			std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> indexBufferView;
			std::unique_ptr<D3D12_CONSTANT_BUFFER_VIEW_DESC> constantBufferViewDesc;
			std::unique_ptr<D3D12_RENDER_TARGET_VIEW_DESC> renderTargetViewDesc;
			std::unique_ptr<D3D12_DEPTH_STENCIL_VIEW_DESC> depthStencilViewDesc;
			std::unique_ptr<D3D12_SAMPLER_DESC> samplerDesc;
			std::unique_ptr<D3D12_UNORDERED_ACCESS_VIEW_DESC> unorderedAccessViewDesc;
		};
	};

	class IResourceFactory
	{
	public:
		virtual ~IResourceFactory() = 0 {};
		virtual ResourceData CreateResource(ComPtr<ID3D12GraphicsCommandList> commandList, ResourceType type, const ResourceDesc& desc) = 0;
	};
}
