#include "ComputeObjectBuilder.h"

Graphics::Assets::ComputeObjectBuilder::ComputeObjectBuilder()
	: computeShader{}
{

}

Graphics::Assets::ComputeObjectBuilder::~ComputeObjectBuilder()
{
	Reset();
}

void Graphics::Assets::ComputeObjectBuilder::SetRootConstants(uint32_t registerIndex,
	uint32_t constantsNumber)
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameter.Constants.RegisterSpace = 0;
	rootParameter.Constants.ShaderRegister = registerIndex;
	rootParameter.Constants.Num32BitValues = constantsNumber;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	rootConstantIndices.insert({ registerIndex, rootParameterIndex });

	rootParameters.push_back(std::move(rootParameter));
}

void Graphics::Assets::ComputeObjectBuilder::SetConstantBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_CBV, gpuAddress, constantBufferSlots);
}

void Graphics::Assets::ComputeObjectBuilder::SetTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gpuDescriptor);
}

void Graphics::Assets::ComputeObjectBuilder::SetBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_SRV, gpuAddress, bufferSlots);
}

void Graphics::Assets::ComputeObjectBuilder::SetRWTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, gpuDescriptor);
}

void Graphics::Assets::ComputeObjectBuilder::SetRWBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_UAV, gpuAddress, rwBufferSlots);
}

void Graphics::Assets::ComputeObjectBuilder::SetSampler(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, gpuDescriptor);
}

void Graphics::Assets::ComputeObjectBuilder::SetShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	computeShader = shaderBytecode;
}

Graphics::Assets::ComputeObject* Graphics::Assets::ComputeObjectBuilder::Compose(ID3D12Device* device)
{
	auto rootSignatureFlags = SetRootSignatureFlags();
	auto rootSignature = CreateRootSignature(device, rootSignatureFlags);
	auto pipelineState = CreateComputePipelineState(device, rootSignature);

	auto newComputeObject = new ComputeObject(rootSignature, pipelineState, rootConstantIndices,
		constantBufferSlots, bufferSlots, rwBufferSlots, textureSlots);

	Reset();

	return newComputeObject;
}

void Graphics::Assets::ComputeObjectBuilder::Reset()
{
	computeShader = {};

	rootConstantIndices.clear();
	constantBufferSlots.clear();
	bufferSlots.clear();
	rwBufferSlots.clear();
	textureSlots.clear();

	for (auto& parameter : rootParameters)
		if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			delete parameter.DescriptorTable.pDescriptorRanges;

	rootParameters.clear();
}

void Graphics::Assets::ComputeObjectBuilder::SetDescriptorParameter(uint32_t registerIndex,
	D3D12_ROOT_PARAMETER_TYPE parameterType, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, std::vector<DescriptorSlot>& slots)
{
	DescriptorSlot slot{};
	slot.shaderRegisterIndex = registerIndex;
	slot.rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	slot.gpuAddress = gpuAddress;

	slots.push_back(std::move(slot));

	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = parameterType;
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.Descriptor.ShaderRegister = registerIndex;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters.push_back(std::move(rootParameter));
}

void Graphics::Assets::ComputeObjectBuilder::SetDescriptorTableParameter(uint32_t registerIndex,
	D3D12_DESCRIPTOR_RANGE_TYPE rangeType, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	DescriptorTableSlot slot{};
	slot.rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	slot.gpuDescriptor = gpuDescriptor;

	textureSlots.push_back(std::move(slot));

	D3D12_DESCRIPTOR_RANGE* descriptorRange = new D3D12_DESCRIPTOR_RANGE;
	descriptorRange->RangeType = rangeType;
	descriptorRange->NumDescriptors = 1u;
	descriptorRange->BaseShaderRegister = registerIndex;
	descriptorRange->OffsetInDescriptorsFromTableStart = 0u;
	descriptorRange->RegisterSpace = 0u;

	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.DescriptorTable.NumDescriptorRanges = 1u;
	rootParameter.DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters.push_back(std::move(rootParameter));
}

D3D12_ROOT_SIGNATURE_FLAGS Graphics::Assets::ComputeObjectBuilder::SetRootSignatureFlags()
{
	D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	
	return flags;
}

ID3D12RootSignature* Graphics::Assets::ComputeObjectBuilder::CreateRootSignature(ID3D12Device* device,
	D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	ID3D12RootSignature* rootSignature = nullptr;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = static_cast<uint32_t>(rootParameters.size());

	if (rootSignatureDesc.NumParameters > 0)
		rootSignatureDesc.pParameters = rootParameters.data();

	rootSignatureDesc.Flags = flags;

	ID3DBlob* signature;
	ID3DBlob* error;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);

	if (error != nullptr)
	{
		OutputDebugStringA(reinterpret_cast<char*>(error->GetBufferPointer()));

		error->Release();
	}

	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	signature->Release();

	return rootSignature;
}

ID3D12PipelineState* Graphics::Assets::ComputeObjectBuilder::CreateComputePipelineState(ID3D12Device* device,
	ID3D12RootSignature* rootSignature)
{
	ID3D12PipelineState* pipelineState = nullptr;

	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = rootSignature;
	pipelineStateDesc.CS = computeShader;

	device->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState));

	return pipelineState;
}
