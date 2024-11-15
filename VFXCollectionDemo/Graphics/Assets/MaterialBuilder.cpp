#include "MaterialBuilder.h"
#include "../DirectX12Utilities.h"

Graphics::Assets::MaterialBuilder::MaterialBuilder()
	: vertexShader{}, hullShader{}, domainShader{}, geometryShader{}, amplificationShader{}, meshShader{}, pixelShader{},
	topologyType{}, cullMode(D3D12_CULL_MODE_NONE), blendDesc{}, renderTargetNumber(0u), depthStencilFormat{}, zTest(false),
	depthBufferReadOnly(false), _depthBias(0.0f), _conservativeRender(false)
{
	renderTargetsFormat.fill(DXGI_FORMAT_UNKNOWN);
}

Graphics::Assets::MaterialBuilder::~MaterialBuilder()
{
	Reset();
}

void Graphics::Assets::MaterialBuilder::SetRootConstants(uint32_t registerIndex, uint32_t constantsNumber,
	D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameter.Constants.RegisterSpace = 0;
	rootParameter.Constants.ShaderRegister = registerIndex;
	rootParameter.Constants.Num32BitValues = constantsNumber;
	rootParameter.ShaderVisibility = visibility;

	auto rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	rootConstantIndices.insert({ registerIndex, rootParameterIndex });

	rootParameters.push_back(std::move(rootParameter));
}

void Graphics::Assets::MaterialBuilder::SetConstantBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_CBV, gpuAddress, visibility, constantBufferSlots);
}

void Graphics::Assets::MaterialBuilder::SetTexture(uint32_t registerIndex, 
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gpuDescriptor, visibility);
}

void Graphics::Assets::MaterialBuilder::SetBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_SRV, gpuAddress, visibility, bufferSlots);
}

void Graphics::Assets::MaterialBuilder::SetRWTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, gpuDescriptor, visibility);
}

void Graphics::Assets::MaterialBuilder::SetRWBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_UAV, gpuAddress, visibility, rwBufferSlots);
}

void Graphics::Assets::MaterialBuilder::SetSampler(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, D3D12_SHADER_VISIBILITY visibility)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, gpuDescriptor, visibility);
}

void Graphics::Assets::MaterialBuilder::SetVertexShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	vertexShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetHullShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	hullShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetDomainShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	domainShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetGeometryShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	geometryShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetAmplificationShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	amplificationShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetMeshShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	meshShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetPixelShader(D3D12_SHADER_BYTECODE shaderBytecode)
{
	pixelShader = shaderBytecode;
}

void Graphics::Assets::MaterialBuilder::SetGeometryFormat(VertexFormat format, D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	topologyType = type;

	uint32_t appendParameter = D3D12_APPEND_ALIGNED_ELEMENT;
	auto classification = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

	if ((format & VertexFormat::POSITION) == VertexFormat::POSITION)
		inputElements.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::NORMAL) == VertexFormat::NORMAL)
		inputElements.push_back({ "NORMAL", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TANGENT) == VertexFormat::TANGENT)
		inputElements.push_back({ "TANGENT", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR0) == VertexFormat::COLOR0)
		inputElements.push_back({ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR1) == VertexFormat::COLOR1)
		inputElements.push_back({ "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR2) == VertexFormat::COLOR2)
		inputElements.push_back({ "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR3) == VertexFormat::COLOR3)
		inputElements.push_back({ "COLOR", 3, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR4) == VertexFormat::COLOR4)
		inputElements.push_back({ "COLOR", 4, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR5) == VertexFormat::COLOR5)
		inputElements.push_back({ "COLOR", 5, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR6) == VertexFormat::COLOR6)
		inputElements.push_back({ "COLOR", 6, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::COLOR7) == VertexFormat::COLOR7)
		inputElements.push_back({ "COLOR", 7, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD0) == VertexFormat::TEXCOORD0)
		inputElements.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD1) == VertexFormat::TEXCOORD1)
		inputElements.push_back({ "TEXCOORD", 1, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD2) == VertexFormat::TEXCOORD2)
		inputElements.push_back({ "TEXCOORD", 2, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD3) == VertexFormat::TEXCOORD3)
		inputElements.push_back({ "TEXCOORD", 3, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD4) == VertexFormat::TEXCOORD4)
		inputElements.push_back({ "TEXCOORD", 4, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD5) == VertexFormat::TEXCOORD5)
		inputElements.push_back({ "TEXCOORD", 5, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD6) == VertexFormat::TEXCOORD6)
		inputElements.push_back({ "TEXCOORD", 6, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::TEXCOORD7) == VertexFormat::TEXCOORD7)
		inputElements.push_back({ "TEXCOORD", 7, DXGI_FORMAT_R16G16_FLOAT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::BLENDINDICES) == VertexFormat::BLENDINDICES)
		inputElements.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, appendParameter, classification, 0 });

	if ((format & VertexFormat::BLENDWEIGHT) == VertexFormat::BLENDWEIGHT)
		inputElements.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, appendParameter, classification, 0 });
}

void Graphics::Assets::MaterialBuilder::SetDepthBias(float depthBias)
{
	_depthBias = depthBias;
}

void Graphics::Assets::MaterialBuilder::SetConservativeRender()
{
	_conservativeRender = true;
}

void Graphics::Assets::MaterialBuilder::SetCullMode(D3D12_CULL_MODE mode)
{
	cullMode = mode;
}

void Graphics::Assets::MaterialBuilder::SetBlendMode(D3D12_BLEND_DESC desc)
{
	blendDesc = desc;
}

void Graphics::Assets::MaterialBuilder::SetRenderTargetFormat(uint32_t renderTargetIndex, DXGI_FORMAT format)
{
	renderTargetsFormat[renderTargetIndex] = format;
	renderTargetNumber++;
}

void Graphics::Assets::MaterialBuilder::SetDepthStencilFormat(uint32_t depthBit, bool enableZTest,
	bool readOnly, bool isPrepass)
{
	depthStencilFormat = (depthBit == 32) ? DXGI_FORMAT_D32_FLOAT : (depthBit == 16) ? DXGI_FORMAT_D24_UNORM_S8_UINT :
		DXGI_FORMAT_UNKNOWN;
	zTest = enableZTest;
	depthBufferReadOnly = readOnly;
	isZPrepass = isPrepass;
}

Graphics::Assets::Material* Graphics::Assets::MaterialBuilder::ComposeStandard(ID3D12Device* device)
{
	auto rootSignatureFlags = SetRootSignatureFlags(true);
	auto rootSignature = CreateRootSignature(device, rootSignatureFlags);
	auto pipelineState = CreateGraphicsPipelineState(device, rootSignature);

	auto newMaterial = new Material(rootSignature, pipelineState, rootConstantIndices,
		constantBufferSlots, bufferSlots, rwBufferSlots, textureSlots);

	Reset();

	return newMaterial;
}

Graphics::Assets::Material* Graphics::Assets::MaterialBuilder::ComposeMeshletized(ID3D12Device2* device)
{
	auto rootSignatureFlags = SetRootSignatureFlags(false);
	auto rootSignature = CreateRootSignature(device, rootSignatureFlags);
	auto pipelineState = CreatePipelineState(device, rootSignature);

	auto newMaterial = new Material(rootSignature, pipelineState, rootConstantIndices,
		constantBufferSlots, bufferSlots, rwBufferSlots, textureSlots);

	Reset();

	return newMaterial;
}

void Graphics::Assets::MaterialBuilder::Reset()
{
	vertexShader = {};
	hullShader = {};
	domainShader = {};
	geometryShader = {};

	amplificationShader = {};
	meshShader = {};

	pixelShader = {};

	topologyType = {};
	cullMode = D3D12_CULL_MODE_NONE;
	blendDesc = {};
	depthStencilFormat = {};

	_depthBias = D3D12_DEFAULT_DEPTH_BIAS;
	_conservativeRender = false;
	zTest = false;
	depthBufferReadOnly = false;
	renderTargetNumber = 0u;

	renderTargetsFormat.fill(DXGI_FORMAT_UNKNOWN);

	rootConstantIndices.clear();
	constantBufferSlots.clear();
	bufferSlots.clear();
	rwBufferSlots.clear();
	textureSlots.clear();

	for (auto& parameter : rootParameters)
		if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			delete parameter.DescriptorTable.pDescriptorRanges;

	rootParameters.clear();

	inputElements.clear();
}

void Graphics::Assets::MaterialBuilder::SetDescriptorParameter(uint32_t registerIndex,
	D3D12_ROOT_PARAMETER_TYPE parameterType, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
	D3D12_SHADER_VISIBILITY visibility, std::vector<DescriptorSlot>& slots)
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
	rootParameter.ShaderVisibility = visibility;

	rootParameters.push_back(std::move(rootParameter));
}

void Graphics::Assets::MaterialBuilder::SetDescriptorTableParameter(uint32_t registerIndex,
	D3D12_DESCRIPTOR_RANGE_TYPE rangeType, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, D3D12_SHADER_VISIBILITY visibility)
{
	DescriptorTableSlot slot{};

	if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
		slot.tableType = DescriptorTableType::TEXTURE;
	else if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
		slot.tableType = DescriptorTableType::RW_TEXTURE;
	else
		slot.tableType = DescriptorTableType::SAMPLER;

	slot.shaderRegisterIndex = registerIndex;
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
	rootParameter.ShaderVisibility = visibility;

	rootParameters.push_back(std::move(rootParameter));
}

D3D12_ROOT_SIGNATURE_FLAGS Graphics::Assets::MaterialBuilder::SetRootSignatureFlags(bool isStandardPipeline)
{
	D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	if (isStandardPipeline)
	{
		if (inputElements.size() > 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		if (vertexShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		if (hullShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		if (domainShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		if (geometryShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		if (pixelShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	}
	else
	{
		if (amplificationShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;

		if (meshShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

		if (pixelShader.BytecodeLength == 0u)
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	}

	return flags;
}

ID3D12RootSignature* Graphics::Assets::MaterialBuilder::CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags)
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

ID3D12PipelineState* Graphics::Assets::MaterialBuilder::CreateGraphicsPipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
	ID3D12PipelineState* pipelineState = nullptr;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.NumElements = static_cast<uint32_t>(inputElements.size());
	inputLayoutDesc.pInputElementDescs = inputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.InputLayout = inputLayoutDesc;
	pipelineStateDesc.pRootSignature = rootSignature;
	pipelineStateDesc.RasterizerState = DirectX12Utilities::CreateRasterizeDesc(cullMode, _depthBias, _conservativeRender);
	pipelineStateDesc.BlendState = blendDesc;
	pipelineStateDesc.VS = vertexShader;
	pipelineStateDesc.HS = hullShader;
	pipelineStateDesc.DS = domainShader;
	pipelineStateDesc.GS = geometryShader;
	pipelineStateDesc.PS = pixelShader;
	pipelineStateDesc.DepthStencilState = DirectX12Utilities::CreateDepthStencilDesc(zTest, depthBufferReadOnly, isZPrepass);
	pipelineStateDesc.DSVFormat = depthStencilFormat;
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = topologyType;
	pipelineStateDesc.NumRenderTargets = renderTargetNumber;

	if (renderTargetNumber > 0u)
	{
		auto srcAddress = renderTargetsFormat.data();
		auto destAddress = &pipelineStateDesc.RTVFormats[0];
		std::copy(srcAddress, srcAddress + renderTargetNumber, destAddress);
	}

	pipelineStateDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState));

	return pipelineState;
}

ID3D12PipelineState* Graphics::Assets::MaterialBuilder::CreatePipelineState(ID3D12Device2* device, ID3D12RootSignature* rootSignature)
{
	ID3D12PipelineState* pipelineState = nullptr;

	PIPELINE_STATE_STREAM_DESC pipelineStateDesc{};
	pipelineStateDesc.rootSignature = rootSignature;
	pipelineStateDesc.rasterizerState = DirectX12Utilities::CreateRasterizeDesc(cullMode, _depthBias, _conservativeRender);
	pipelineStateDesc.blendDesc = blendDesc;
	pipelineStateDesc.meshShader = meshShader;
	pipelineStateDesc.amplificationShader = amplificationShader;
	pipelineStateDesc.pixelShader = pixelShader;
	pipelineStateDesc.depthStencilDesc = DirectX12Utilities::CreateDepthStencilDesc1(zTest, depthBufferReadOnly, isZPrepass);
	pipelineStateDesc.dsvFormat = depthStencilFormat;
	pipelineStateDesc.sampleMask = UINT_MAX;
	pipelineStateDesc.sampleDesc = { 1, 0 };
	
	D3D12_RT_FORMAT_ARRAY rtvFormatsArray{};
	rtvFormatsArray.NumRenderTargets = renderTargetNumber;

	if (renderTargetNumber > 0u)
	{
		auto srcAddress = renderTargetsFormat.data();
		auto destAddress = &rtvFormatsArray.RTFormats[0];
		std::copy(srcAddress, srcAddress + renderTargetNumber, destAddress);
	}

	pipelineStateDesc.rtvFormats = rtvFormatsArray;

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
	streamDesc.SizeInBytes = sizeof(pipelineStateDesc);
	streamDesc.pPipelineStateSubobjectStream = &pipelineStateDesc;

	device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState));

	return pipelineState;
}
