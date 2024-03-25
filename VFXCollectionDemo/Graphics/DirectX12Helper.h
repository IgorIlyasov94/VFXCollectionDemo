#pragma once

#include "../Includes.h"

namespace Graphics
{
	using TextureInfo = struct
	{
		uint64_t width;
		uint32_t height;
		uint16_t depth;
		uint16_t mipLevels;
		uint64_t rowPitch;
		uint64_t slicePitch;
		DXGI_FORMAT format;
		D3D12_RESOURCE_DIMENSION dimension;
		D3D12_SRV_DIMENSION srvDimension;
	};

	static constexpr size_t _KB = 1024;
	static constexpr size_t _MB = 1024 * _KB;
	static constexpr size_t _GB = 1024 * _MB;

	template <typename DataType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultType = DataType>
	class alignas(void*) PIPELINE_STATE_STREAM_SUBOBJECT
	{
	public:
		PIPELINE_STATE_STREAM_SUBOBJECT() noexcept
			: _type(Type), _data(DefaultType{})
		{

		}

		PIPELINE_STATE_STREAM_SUBOBJECT(DataType const& data)
			: _type(Type), _data(data)
		{

		}

		PIPELINE_STATE_STREAM_SUBOBJECT& operator=(DataType const& data)
		{
			_data = data;
			return *this;
		}

		operator DataType const& () const
		{
			return _data;
		}

		operator DataType& ()
		{
			return _data;
		}

		DataType* operator&()
		{
			return &_data;
		}

		DataType const* operator&() const
		{
			return &_data;
		}

	private:
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _type;
		DataType _data;
	};

	struct PIPELINE_STATE_STREAM_DESC
	{
	public:
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS> flags;
		PIPELINE_STATE_STREAM_SUBOBJECT<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK> nodeMask;
		PIPELINE_STATE_STREAM_SUBOBJECT<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> rootSignature;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> pixelShader;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS> amplificationShader;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS> meshShader;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND> blendDesc;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_DEPTH_STENCIL_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1> depthStencilDesc;
		PIPELINE_STATE_STREAM_SUBOBJECT<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> dsvFormat;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> rasterizerState;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> rtvFormats;
		PIPELINE_STATE_STREAM_SUBOBJECT<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC> sampleDesc;
		PIPELINE_STATE_STREAM_SUBOBJECT<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK> sampleMask;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO> cachedPipelineState;
		PIPELINE_STATE_STREAM_SUBOBJECT<D3D12_VIEW_INSTANCING_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING> viewInstancingDesc;
	};

	class DirectX12Helper final
	{
	public:
		~DirectX12Helper();

		static void CreateFactory(IDXGIFactory7** factory);
		static void CreateDevice(IDXGIAdapter1* adapter, ID3D12Device** device);
		static void CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** commandQueue);

		static void CreateSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND& windowHandler,
			const uint32_t buffersCount, const int32_t& resolutionX, const int32_t& resolutionY, IDXGISwapChain1** swapChain);

		static void CreateDescriptorHeap(ID3D12Device* device, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
			D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap** descriptorHeap);

		static void CreateStandardSamplerDescs(std::vector<D3D12_STATIC_SAMPLER_DESC>& samplerDescs);

		static void GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter4** adapter);

		static void SetupRasterizerDesc(D3D12_RASTERIZER_DESC& rasterizerDesc, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE) noexcept;

		static void SetupBlendDesc(D3D12_BLEND_DESC& blendDesc, bool blendOn = false, D3D12_BLEND srcBlend = D3D12_BLEND_ONE,
			D3D12_BLEND destBlend = D3D12_BLEND_ZERO, D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD, D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE,
			D3D12_BLEND destBlendAlpha = D3D12_BLEND_ZERO, D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD) noexcept;

		static void SetupDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& depthStencilDesc, bool depthEnable) noexcept;
		static void SetupResourceBufferDesc(D3D12_RESOURCE_DESC& resourceDesc, uint64_t bufferSize,
			D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE, uint64_t alignment = 0) noexcept;

		static void SetupResourceTextureDesc(D3D12_RESOURCE_DESC& resourceDesc, const TextureInfo& textureInfo,
			D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE, uint64_t alignment = 0) noexcept;

		static void SetupHeapProperties(D3D12_HEAP_PROPERTIES& heapProperties, D3D12_HEAP_TYPE heapType) noexcept;

		static void SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* const resource,
			D3D12_RESOURCE_BARRIER_FLAGS barrierFlags, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

		static void ExecuteGPUCommands(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* allocator,
			ID3D12CommandQueue* queue, ID3D12Fence* fence, HANDLE& fenceEvent, uint64_t& fenceValue);

	private:
		DirectX12Helper() = delete;
		DirectX12Helper(const DirectX12Helper&) = delete;
		DirectX12Helper(DirectX12Helper&&) = delete;
		DirectX12Helper& operator=(const DirectX12Helper&) = delete;
		DirectX12Helper& operator=(DirectX12Helper&&) = delete;
	};
}
