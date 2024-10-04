#pragma once

#include "../DirectX12Includes.h"
#include "../BufferManager.h"
#include "Material.h"

namespace Graphics::Assets
{
	struct RaytracingObjectDesc
	{
		ID3D12RootSignature* globalRootSignature;
		const std::vector<ID3D12RootSignature*>& localRootSignatures;

		ID3D12StateObject* pipelineState;

		const std::map<uint32_t, uint32_t>& rootConstantIndices;
		const std::vector<DescriptorSlot>& constantBufferSlots;
		const std::vector<DescriptorSlot>& bufferSlots;
		const std::vector<DescriptorSlot>& rwBufferSlots;
		const std::vector<DescriptorTableSlot>& textureSlots;

		const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc;

		BufferAllocation shaderTable;
		BufferAllocation bottomLevelStructure;
		BufferAllocation bottomLevelStructureAABB;
		BufferAllocation topLevelStructure;
	};

	class RaytracingObject
	{
	public:
		RaytracingObject(const RaytracingObjectDesc& desc);
		~RaytracingObject();

		void Release(BufferManager* bufferManager);

		void UpdateConstantBuffer(uint32_t cRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateBuffer(uint32_t tRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateRWBuffer(uint32_t uRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);

		void SetRootConstant(ID3D12GraphicsCommandList4* commandList, uint32_t cRegisterIndex, const void* value);
		void SetRootConstants(ID3D12GraphicsCommandList4* commandList, uint32_t cRegisterIndex,
			uint32_t constantNumber, const void* values);

		void Dispatch(ID3D12GraphicsCommandList4* commandList, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);

	private:
		RaytracingObject() = delete;

		D3D12_DISPATCH_RAYS_DESC dispatchDesc;

		ID3D12RootSignature* globalRootSignature;
		std::vector<ID3D12RootSignature*> localRootSignatures;
		ID3D12StateObject* _pipelineState;

		std::map<uint32_t, uint32_t> _rootConstantIndices;
		std::vector<DescriptorSlot> _constantBufferSlots;
		std::vector<DescriptorSlot> _bufferSlots;
		std::vector<DescriptorSlot> _rwBufferSlots;
		std::vector<DescriptorTableSlot> _textureSlots;

		BufferAllocation shaderTable;
		BufferAllocation bottomLevelStructure;
		BufferAllocation bottomLevelStructureAABB;
		BufferAllocation topLevelStructure;
	};
}
