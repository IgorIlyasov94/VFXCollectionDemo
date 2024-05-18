#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "GPUResource.h"

namespace Graphics::Resources
{
	struct IResource
	{
	public:
		virtual ~IResource() = 0 {};
	};

	struct Buffer : public IResource
	{
	public:
		~Buffer() override
		{

		};

		GPUResource* resource;
		uint64_t size;
		D3D12_GPU_VIRTUAL_ADDRESS resourceGPUAddress;
		DescriptorAllocation srvDescriptor;
	};

	struct ConstantBuffer : public IResource
	{
	public:
		~ConstantBuffer() override
		{

		};

		GPUResource* resource;
		uint64_t size;
		uint8_t* resourceCPUAddress;
		D3D12_GPU_VIRTUAL_ADDRESS resourceGPUAddress;
		DescriptorAllocation cbvDescriptor;
	};

	struct DepthStencilTarget : public IResource
	{
	public:
		~DepthStencilTarget() override
		{

		};

		GPUResource* resource;
		DescriptorAllocation srvDescriptor;
		DescriptorAllocation dsvDescriptor;
	};

	struct IndexBuffer : public IResource
	{
	public:
		~IndexBuffer() override
		{

		};

		GPUResource* resource;
		D3D12_INDEX_BUFFER_VIEW viewDesc;
		uint32_t indicesNumber;
	};

	struct RenderTarget : public IResource
	{
	public:
		~RenderTarget() override
		{

		};

		GPUResource* resource;
		DescriptorAllocation srvDescriptor;
		DescriptorAllocation rtvDescriptor;
	};

	struct RWBuffer : public IResource
	{
	public:
		~RWBuffer() override
		{

		};

		GPUResource* resource;
		GPUResource* counterResource;
		uint64_t size;
		D3D12_GPU_VIRTUAL_ADDRESS resourceGPUAddress;
		DescriptorAllocation srvDescriptor;
		DescriptorAllocation uavDescriptor;
		DescriptorAllocation uavNonShaderVisibleDescriptor;
	};

	struct RWTexture : public IResource
	{
	public:
		~RWTexture() override
		{

		};

		GPUResource* resource;
		DescriptorAllocation srvDescriptor;
		DescriptorAllocation uavDescriptor;
		DescriptorAllocation uavNonShaderVisibleDescriptor;
	};

	struct Sampler : public IResource
	{
	public:
		~Sampler() override
		{

		};

		DescriptorAllocation samplerDescriptor;
		D3D12_SAMPLER_DESC samplerDesc;
	};

	struct Texture : public IResource
	{
	public:
		~Texture() override
		{

		};

		GPUResource* resource;
		DescriptorAllocation srvDescriptor;
	};

	struct VertexBuffer : public IResource
	{
	public:
		~VertexBuffer() override
		{

		};

		GPUResource* resource;
		D3D12_VERTEX_BUFFER_VIEW viewDesc;
	};

	struct Shader : public IResource
	{
	public:
		~Shader() override
		{
			delete[] bytecode.pShaderBytecode;
		};

		D3D12_SHADER_BYTECODE bytecode;
	};
}
