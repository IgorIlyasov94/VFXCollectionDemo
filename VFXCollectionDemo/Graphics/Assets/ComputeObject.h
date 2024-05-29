#pragma once

#include "../DirectX12Includes.h"
#include "Material.h"

namespace Graphics::Assets
{
	class ComputeObject
	{
	public:
		ComputeObject(ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
			const std::map<uint32_t, uint32_t>& rootConstantIndices, const std::vector<DescriptorSlot>& constantBufferSlots,
			const std::vector<DescriptorSlot>& bufferSlots, const std::vector<DescriptorSlot>& rwBufferSlots,
			const std::vector<DescriptorTableSlot>& textureSlots);
		~ComputeObject();

		void UpdateConstantBuffer(uint32_t cRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateBuffer(uint32_t tRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateRWBuffer(uint32_t uRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);

		void SetRootConstant(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex, const void* value);
		void SetRootConstants(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex,
			uint32_t constantNumber, const void* values);

		void Set(ID3D12GraphicsCommandList* commandList);
		void Dispatch(ID3D12GraphicsCommandList* commandList, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);

	private:
		ComputeObject() = delete;

		ID3D12RootSignature* _rootSignature;
		ID3D12PipelineState* _pipelineState;

		std::map<uint32_t, uint32_t> _rootConstantIndices;
		std::vector<DescriptorSlot> _constantBufferSlots;
		std::vector<DescriptorSlot> _bufferSlots;
		std::vector<DescriptorSlot> _rwBufferSlots;
		std::vector<DescriptorTableSlot> _textureSlots;
	};
}
