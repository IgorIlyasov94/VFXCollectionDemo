#pragma once

#include "../DirectX12Includes.h"
#include "../BufferManager.h"
#include "Material.h"

namespace Graphics::Assets
{
	struct RaytracingObjectDesc
	{
		ID3D12RootSignature* globalRootSignature;
		std::vector<ID3D12RootSignature*> localRootSignatures;

		ID3D12StateObject* pipelineState;

		std::map<uint32_t, uint32_t> rootConstantIndices;
		std::vector<DescriptorSlot> constantBufferSlots;
		std::vector<DescriptorSlot> bufferSlots;
		std::vector<DescriptorSlot> rwBufferSlots;
		std::vector<DescriptorTableSlot> textureSlots;

		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc;

		BufferAllocation shaderTable;
		BufferAllocation bottomLevelStructure;
		BufferAllocation bottomLevelStructureAABB;
		BufferAllocation topLevelStructure;
	};

	class RaytracingObject
	{
	public:
		RaytracingObject(RaytracingObjectDesc&& desc);
		~RaytracingObject();

		void Release(BufferManager* bufferManager);

		void UpdateConstantBuffer(uint32_t cRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateBuffer(uint32_t tRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateRWBuffer(uint32_t uRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress);
		void UpdateTable(uint32_t registerIndex, DescriptorTableType tableType, D3D12_GPU_DESCRIPTOR_HANDLE newGPUDescriptorHandle);

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
