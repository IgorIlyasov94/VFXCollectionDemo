#pragma once

#include "../DirectX12Includes.h"

namespace Graphics::Assets
{
	struct DescriptorSlot
	{
		uint32_t rootParameterIndex;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

	struct DescriptorTableSlot
	{
		uint32_t rootParameterIndex;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor;
	};

	class Material
	{
	public:
		Material(ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState, const std::vector<DescriptorSlot>& constantBufferSlots,
			const std::vector<DescriptorSlot>& bufferSlots, const std::vector<DescriptorSlot>& rwBufferSlots,
			const std::vector<DescriptorTableSlot>& textureSlots);
		~Material();

		void Set(ID3D12GraphicsCommandList* commandList);

	private:
		Material() = delete;

		ID3D12RootSignature* _rootSignature;
		ID3D12PipelineState* _pipelineState;

		std::vector<DescriptorSlot> _constantBufferSlots;
		std::vector<DescriptorSlot> _bufferSlots;
		std::vector<DescriptorSlot> _rwBufferSlots;
		std::vector<DescriptorTableSlot> _textureSlots;
	};
}
