#include "RaytracingObject.h"

Graphics::Assets::RaytracingObject::RaytracingObject(RaytracingObjectDesc&& desc)
	: dispatchDesc(desc.dispatchRaysDesc), globalRootSignature(desc.globalRootSignature),
	_pipelineState(desc.pipelineState), localRootSignatures{}, _rootConstantIndices{},
	_constantBufferSlots{}, _bufferSlots{}, _rwBufferSlots{}, _textureSlots{},
	shaderTable{}, bottomLevelStructure{}, bottomLevelStructureAABB{}, topLevelStructure{}
{
	if (!desc.localRootSignatures.empty())
		localRootSignatures = std::move(desc.localRootSignatures);

	if (!desc.rootConstantIndices.empty())
		_rootConstantIndices = std::move(desc.rootConstantIndices);

	if (!desc.constantBufferSlots.empty())
		_constantBufferSlots = std::move(desc.constantBufferSlots);

	if (!desc.bufferSlots.empty())
		_bufferSlots = std::move(desc.bufferSlots);

	if (!desc.rwBufferSlots.empty())
		_rwBufferSlots = std::move(desc.rwBufferSlots);

	if (!desc.textureSlots.empty())
		_textureSlots = std::move(desc.textureSlots);

	shaderTable = std::move(desc.shaderTable);
	bottomLevelStructure = std::move(desc.bottomLevelStructure);
	bottomLevelStructureAABB = std::move(desc.bottomLevelStructureAABB);
	topLevelStructure = std::move(desc.topLevelStructure);
}

Graphics::Assets::RaytracingObject::~RaytracingObject()
{
	
}

void Graphics::Assets::RaytracingObject::Release(BufferManager* bufferManager)
{
	_pipelineState->Release();
	globalRootSignature->Release();

	for (auto& rootSignature : localRootSignatures)
		rootSignature->Release();

	bufferManager->Deallocate(shaderTable.resource,
		shaderTable.gpuAddress, shaderTable.size);

	if (bottomLevelStructure.size > 0ull)
		bufferManager->Deallocate(bottomLevelStructure.resource,
			bottomLevelStructure.gpuAddress, bottomLevelStructure.size);

	if (bottomLevelStructureAABB.size > 0ull)
		bufferManager->Deallocate(bottomLevelStructureAABB.resource,
			bottomLevelStructureAABB.gpuAddress, bottomLevelStructureAABB.size);

	bufferManager->Deallocate(topLevelStructure.resource,
		topLevelStructure.gpuAddress, topLevelStructure.size);
}

void Graphics::Assets::RaytracingObject::UpdateConstantBuffer(uint32_t cRegisterIndex,
	D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _constantBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == cRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::RaytracingObject::UpdateBuffer(uint32_t tRegisterIndex,
	D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _bufferSlots)
		if (descriptorSlot.shaderRegisterIndex == tRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::RaytracingObject::UpdateRWBuffer(uint32_t uRegisterIndex,
	D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _rwBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == uRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::RaytracingObject::UpdateTable(uint32_t registerIndex, DescriptorTableType tableType,
	D3D12_GPU_DESCRIPTOR_HANDLE newGPUDescriptorHandle)
{
	for (auto& descriptorTableSlot : _textureSlots)
		if (descriptorTableSlot.shaderRegisterIndex == registerIndex &&
			descriptorTableSlot.tableType == tableType)
		{
			descriptorTableSlot.gpuDescriptor = newGPUDescriptorHandle;
			return;
		}
}

void Graphics::Assets::RaytracingObject::SetRootConstant(ID3D12GraphicsCommandList4* commandList,
	uint32_t cRegisterIndex, const void* value)
{
	commandList->SetComputeRoot32BitConstant(_rootConstantIndices[cRegisterIndex],
		*reinterpret_cast<const uint32_t*>(value), 0u);
}

void Graphics::Assets::RaytracingObject::SetRootConstants(ID3D12GraphicsCommandList4* commandList,
	uint32_t cRegisterIndex, uint32_t constantNumber, const void* values)
{
	commandList->SetComputeRoot32BitConstants(_rootConstantIndices[cRegisterIndex],
		constantNumber, values, 0u);
}

void Graphics::Assets::RaytracingObject::Dispatch(ID3D12GraphicsCommandList4* commandList,
	uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
	commandList->SetPipelineState1(_pipelineState);
	commandList->SetComputeRootSignature(globalRootSignature);

	uint32_t rootParameterIndex{};

	for (auto& constantBufferSlot : _constantBufferSlots)
		commandList->SetComputeRootConstantBufferView(constantBufferSlot.rootParameterIndex, constantBufferSlot.gpuAddress);

	for (auto& bufferSlot : _bufferSlots)
		commandList->SetComputeRootShaderResourceView(bufferSlot.rootParameterIndex, bufferSlot.gpuAddress);

	for (auto& rwBufferSlot : _rwBufferSlots)
		commandList->SetComputeRootUnorderedAccessView(rwBufferSlot.rootParameterIndex, rwBufferSlot.gpuAddress);

	for (auto& textureSlot : _textureSlots)
		commandList->SetComputeRootDescriptorTable(textureSlot.rootParameterIndex, textureSlot.gpuDescriptor);

	dispatchDesc.Width = numGroupsX;
	dispatchDesc.Height = numGroupsY;
	dispatchDesc.Depth = numGroupsZ;

	commandList->DispatchRays(&dispatchDesc);
}
