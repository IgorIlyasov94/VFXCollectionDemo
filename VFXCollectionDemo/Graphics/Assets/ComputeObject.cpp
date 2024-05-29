#include "ComputeObject.h"

Graphics::Assets::ComputeObject::ComputeObject(ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
	const std::map<uint32_t, uint32_t>& rootConstantIndices, const std::vector<DescriptorSlot>& constantBufferSlots,
	const std::vector<DescriptorSlot>& bufferSlots, const std::vector<DescriptorSlot>& rwBufferSlots,
	const std::vector<DescriptorTableSlot>& textureSlots)
	: _rootSignature(rootSignature), _pipelineState(pipelineState), _rootConstantIndices(rootConstantIndices),
	_constantBufferSlots(constantBufferSlots), _bufferSlots(bufferSlots), _rwBufferSlots(rwBufferSlots),
	_textureSlots(textureSlots)
{

}

Graphics::Assets::ComputeObject::~ComputeObject()
{
	_pipelineState->Release();
	_rootSignature->Release();
}

void Graphics::Assets::ComputeObject::UpdateConstantBuffer(uint32_t cRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _constantBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == cRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::ComputeObject::UpdateBuffer(uint32_t tRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _bufferSlots)
		if (descriptorSlot.shaderRegisterIndex == tRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::ComputeObject::UpdateRWBuffer(uint32_t uRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _rwBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == uRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::ComputeObject::SetRootConstant(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex,
	const void* value)
{
	commandList->SetComputeRoot32BitConstant(_rootConstantIndices[cRegisterIndex],
		*reinterpret_cast<const uint32_t*>(value), 0u);
}

void Graphics::Assets::ComputeObject::SetRootConstants(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex,
	uint32_t constantNumber, const void* values)
{
	commandList->SetComputeRoot32BitConstants(_rootConstantIndices[cRegisterIndex],
		constantNumber, values, 0u);
}

void Graphics::Assets::ComputeObject::Set(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(_pipelineState);
	commandList->SetComputeRootSignature(_rootSignature);

	uint32_t rootParameterIndex{};

	for (auto& constantBufferSlot : _constantBufferSlots)
		commandList->SetComputeRootConstantBufferView(constantBufferSlot.rootParameterIndex, constantBufferSlot.gpuAddress);

	for (auto& bufferSlot : _bufferSlots)
		commandList->SetComputeRootShaderResourceView(bufferSlot.rootParameterIndex, bufferSlot.gpuAddress);

	for (auto& rwBufferSlot : _rwBufferSlots)
		commandList->SetComputeRootUnorderedAccessView(rwBufferSlot.rootParameterIndex, rwBufferSlot.gpuAddress);

	for (auto& textureSlot : _textureSlots)
		commandList->SetComputeRootDescriptorTable(textureSlot.rootParameterIndex, textureSlot.gpuDescriptor);
}

void Graphics::Assets::ComputeObject::Dispatch(ID3D12GraphicsCommandList* commandList, uint32_t numGroupsX,
	uint32_t numGroupsY, uint32_t numGroupsZ)
{
	commandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}
