#pragma once

#include "../DirectX12Includes.h"
#include "../Resources/IResource.h"
#include "../VertexFormat.h"
#include "Material.h"

namespace Graphics::Assets
{
	class MaterialBuilder
	{
	public:
		MaterialBuilder();
		~MaterialBuilder();

		void SetRootConstants(uint32_t registerIndex, uint32_t constantsNumber,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetConstantBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetRWTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetRWBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void SetSampler(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

		void SetVertexShader(D3D12_SHADER_BYTECODE shaderBytecode);
		void SetHullShader(D3D12_SHADER_BYTECODE shaderBytecode);
		void SetDomainShader(D3D12_SHADER_BYTECODE shaderBytecode);
		void SetGeometryShader(D3D12_SHADER_BYTECODE shaderBytecode);

		void SetAmplificationShader(D3D12_SHADER_BYTECODE shaderBytecode);
		void SetMeshShader(D3D12_SHADER_BYTECODE shaderBytecode);

		void SetPixelShader(D3D12_SHADER_BYTECODE shaderBytecode);

		void SetGeometryFormat(VertexFormat format, D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
		void SetCullMode(D3D12_CULL_MODE mode);
		void SetBlendMode(D3D12_BLEND_DESC desc);
		void SetRenderTargetFormat(uint32_t renderTargetIndex, DXGI_FORMAT format);
		void SetDepthStencilFormat(uint32_t depthBit, bool enableZTest);
		
		Material* ComposeStandard(ID3D12Device* device);
		Material* ComposeMeshletized(ID3D12Device2* device);

		void Reset();

	private:
		MaterialBuilder(const MaterialBuilder&) = delete;
		MaterialBuilder(MaterialBuilder&&) = delete;
		MaterialBuilder& operator=(const MaterialBuilder&) = delete;
		MaterialBuilder& operator=(MaterialBuilder&&) = delete;

		void SetDescriptorParameter(uint32_t registerIndex, D3D12_ROOT_PARAMETER_TYPE parameterType,
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, D3D12_SHADER_VISIBILITY visibility, std::vector<DescriptorSlot>& slots);
		void SetDescriptorTableParameter(uint32_t registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, D3D12_SHADER_VISIBILITY visibility);

		D3D12_ROOT_SIGNATURE_FLAGS SetRootSignatureFlags(bool isStandardPipeline);

		ID3D12RootSignature* CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags);
		ID3D12PipelineState* CreateGraphicsPipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature);
		ID3D12PipelineState* CreatePipelineState(ID3D12Device2* device, ID3D12RootSignature* rootSignature);

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

		D3D12_SHADER_BYTECODE vertexShader;
		D3D12_SHADER_BYTECODE hullShader;
		D3D12_SHADER_BYTECODE domainShader;
		D3D12_SHADER_BYTECODE geometryShader;

		D3D12_SHADER_BYTECODE amplificationShader;
		D3D12_SHADER_BYTECODE meshShader;

		D3D12_SHADER_BYTECODE pixelShader;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType;
		D3D12_CULL_MODE cullMode;
		D3D12_BLEND_DESC blendDesc;
		DXGI_FORMAT depthStencilFormat;

		bool zTest;
		uint32_t renderTargetNumber;

		std::array<DXGI_FORMAT, 8> renderTargetsFormat;

		std::map<uint32_t, uint32_t> rootConstantIndices;
		std::vector<DescriptorSlot> constantBufferSlots;
		std::vector<DescriptorSlot> bufferSlots;
		std::vector<DescriptorSlot> rwBufferSlots;
		std::vector<DescriptorTableSlot> textureSlots;
		
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	};
}
