#pragma once

#include "..\..\Includes.h"
#include "ResourceFactory\VertexBufferFactory.h"
#include "ResourceFactory\IndexBufferFactory.h"
#include "ResourceFactory\ConstantBufferFactory.h"
#include "ResourceFactory\TextureFactory.h"
#include "ResourceFactory\BufferFactory.h"
#include "ResourceFactory\SamplerFactory.h"
#include "ResourceFactory\RenderTargetFactory.h"
#include "ResourceFactory\DepthStencilFactory.h"
#include "ResourceFactory\RWTextureFactory.h"
#include "ResourceFactory\RWBufferFactory.h"
#include "ResourceFactory\SwapChainBufferFactory.h"

namespace Memory
{
	struct RWTexture
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
		Graphics::TextureInfo info;
		TextureAllocation textureAllocation;
		DescriptorAllocation shaderResourceDescriptorAllocation;
		DescriptorAllocation unorderedAccessDescriptorAllocation;
		DescriptorAllocation shaderNonVisibleDescriptorAllocation;
		D3D12_RESOURCE_STATES currentResourceState;
	};

	struct RWBuffer
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
		BufferAllocation bufferAllocation;
		DescriptorAllocation shaderResourceDescriptorAllocation;
		DescriptorAllocation unorderedAccessDescriptorAllocation;
		DescriptorAllocation shaderNonVisibleDescriptorAllocation;
		D3D12_RESOURCE_STATES currentResourceState;
	};

	class ResourceManager
	{
	public:
		ResourceManager(ID3D12Device* _device);
		~ResourceManager();

		ResourceId CreateResource(ResourceType resourceType, const ResourceDesc& desc);
		void ReleaseResource(const ResourceId& resourceId);
		void UpdateResources();

		const ResourceData& GetResourceData(const ResourceId& resourceId) const;

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(const ResourceId& resourceId) const;
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(const ResourceId& resourceId) const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetDescriptorBase(const ResourceId& resourceId) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilDescriptorBase(const ResourceId& resourceId) const;

		ID3D12DescriptorHeap* GetCurrentCbvSrvUavDescriptorHeap();
		ID3D12DescriptorHeap* GetCurrentSamplerDescriptorHeap();

		void UpdateBuffer(const ResourceId& resourceId, const void* data, size_t dataSize);
		void SetResourceBarrier(ID3D12GraphicsCommandList* _commandList, const ResourceId& resourceId,
			D3D12_RESOURCE_BARRIER_FLAGS resourceBarrierFlags, D3D12_RESOURCE_STATES newState);
		void SetUAVBarrier(ID3D12GraphicsCommandList* _commandList, const ResourceId& resourceId);

	private:
		ID3D12Device* device;
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<ID3D12Fence> fence;
		HANDLE fenceEvent;
		uint64_t fenceValue;

		std::shared_ptr<DescriptorAllocator> descriptorAllocator;
		std::shared_ptr<BufferAllocator> bufferAllocator;
		std::shared_ptr<TextureAllocator> textureAllocator;

		std::vector<IResourceFactory*> resourceFactories;
		std::vector<ResourceData> resources;

		std::queue<size_t> freePlaceIndices;
	};
}
