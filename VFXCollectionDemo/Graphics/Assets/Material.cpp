#include "Material.h"

Graphics::Assets::Material::Material(ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
	const std::map<uint32_t, uint32_t>& rootConstantIndices, const std::vector<DescriptorSlot>& constantBufferSlots,
	const std::vector<DescriptorSlot>& bufferSlots, const std::vector<DescriptorSlot>& rwBufferSlots,
	const std::vector<DescriptorTableSlot>& textureSlots)
	: _rootSignature(rootSignature), _pipelineState(pipelineState), _rootConstantIndices(rootConstantIndices),
	_constantBufferSlots(constantBufferSlots), _bufferSlots(bufferSlots), _rwBufferSlots(rwBufferSlots),
	_textureSlots(textureSlots)
{

}

Graphics::Assets::Material::~Material()
{
	_pipelineState->Release();
	_rootSignature->Release();
}

void Graphics::Assets::Material::UpdateConstantBuffer(uint32_t cRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _constantBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == cRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::Material::UpdateBuffer(uint32_t tRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _bufferSlots)
		if (descriptorSlot.shaderRegisterIndex == tRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::Material::UpdateRWBuffer(uint32_t uRegisterIndex, D3D12_GPU_VIRTUAL_ADDRESS newGPUAddress)
{
	for (auto& descriptorSlot : _rwBufferSlots)
		if (descriptorSlot.shaderRegisterIndex == uRegisterIndex)
		{
			descriptorSlot.gpuAddress = newGPUAddress;
			return;
		}
}

void Graphics::Assets::Material::SetRootConstant(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex,
	const void* value)
{
	commandList->SetGraphicsRoot32BitConstant(_rootConstantIndices[cRegisterIndex],
		*reinterpret_cast<const uint32_t*>(value), 0u);
}

void Graphics::Assets::Material::SetRootConstants(ID3D12GraphicsCommandList* commandList, uint32_t cRegisterIndex,
	uint32_t constantNumber, const void* values)
{
	commandList->SetGraphicsRoot32BitConstants(_rootConstantIndices[cRegisterIndex],
		constantNumber, values, 0u);
}

void Graphics::Assets::Material::Set(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(_pipelineState);
	commandList->SetGraphicsRootSignature(_rootSignature);

	for (auto& constantBufferSlot : _constantBufferSlots)
		commandList->SetGraphicsRootConstantBufferView(constantBufferSlot.rootParameterIndex, constantBufferSlot.gpuAddress);

	for (auto& bufferSlot : _bufferSlots)
		commandList->SetGraphicsRootShaderResourceView(bufferSlot.rootParameterIndex, bufferSlot.gpuAddress);

	for (auto& rwBufferSlot : _rwBufferSlots)
		commandList->SetGraphicsRootUnorderedAccessView(rwBufferSlot.rootParameterIndex, rwBufferSlot.gpuAddress);

	for (auto& textureSlot : _textureSlots)
		commandList->SetGraphicsRootDescriptorTable(textureSlot.rootParameterIndex, textureSlot.gpuDescriptor);
}
