#pragma once

#include "../DirectX12Includes.h"
#include "../Resources/IResource.h"
#include "ComputeObject.h"

namespace Graphics::Assets
{
	class ComputeObjectBuilder
	{
	public:
		ComputeObjectBuilder();
		~ComputeObjectBuilder();

		void SetRootConstants(uint32_t registerIndex, uint32_t constantsNumber);
		void SetConstantBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);
		void SetBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetRWTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);
		void SetRWBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetSampler(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);

		void SetShader(D3D12_SHADER_BYTECODE shaderBytecode);

		ComputeObject* Compose(ID3D12Device* device);

		void Reset();

	private:
		ComputeObjectBuilder(const ComputeObjectBuilder&) = delete;
		ComputeObjectBuilder(ComputeObjectBuilder&&) = delete;
		ComputeObjectBuilder& operator=(const ComputeObjectBuilder&) = delete;
		ComputeObjectBuilder& operator=(ComputeObjectBuilder&&) = delete;

		void SetDescriptorParameter(uint32_t registerIndex, D3D12_ROOT_PARAMETER_TYPE parameterType,
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, std::vector<DescriptorSlot>& slots);
		void SetDescriptorTableParameter(uint32_t registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);

		D3D12_ROOT_SIGNATURE_FLAGS SetRootSignatureFlags();

		ID3D12RootSignature* CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags);
		ID3D12PipelineState* CreateComputePipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature);
		
		D3D12_SHADER_BYTECODE computeShader;

		std::map<uint32_t, uint32_t> rootConstantIndices;
		std::vector<DescriptorSlot> constantBufferSlots;
		std::vector<DescriptorSlot> bufferSlots;
		std::vector<DescriptorSlot> rwBufferSlots;
		std::vector<DescriptorTableSlot> textureSlots;
		
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	};
}
