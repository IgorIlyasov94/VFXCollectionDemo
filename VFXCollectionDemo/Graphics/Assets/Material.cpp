#include "Material.h"

Graphics::Assets::Material::Material(ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
	const std::vector<DescriptorSlot>& constantBufferSlots, const std::vector<DescriptorSlot>& bufferSlots,
	const std::vector<DescriptorSlot>& rwBufferSlots, const std::vector<DescriptorTableSlot>& textureSlots)
	: _rootSignature(rootSignature), _pipelineState(pipelineState), _constantBufferSlots(constantBufferSlots),
	_bufferSlots(bufferSlots), _rwBufferSlots(rwBufferSlots), _textureSlots(textureSlots)
{

}

Graphics::Assets::Material::~Material()
{
	_pipelineState->Release();
	_rootSignature->Release();
}

void Graphics::Assets::Material::Set(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(_pipelineState);
	commandList->SetGraphicsRootSignature(_rootSignature);

	uint32_t rootParameterIndex{};

	for (auto& constantBufferSlot : _constantBufferSlots)
		commandList->SetGraphicsRootConstantBufferView(constantBufferSlot.rootParameterIndex, constantBufferSlot.gpuAddress);

	for (auto& bufferSlot : _bufferSlots)
		commandList->SetGraphicsRootShaderResourceView(bufferSlot.rootParameterIndex, bufferSlot.gpuAddress);

	for (auto& rwBufferSlot : _rwBufferSlots)
		commandList->SetGraphicsRootUnorderedAccessView(rwBufferSlot.rootParameterIndex, rwBufferSlot.gpuAddress);

	for (auto& textureSlot : _textureSlots)
		commandList->SetGraphicsRootDescriptorTable(textureSlot.rootParameterIndex, textureSlot.gpuDescriptor);
}
