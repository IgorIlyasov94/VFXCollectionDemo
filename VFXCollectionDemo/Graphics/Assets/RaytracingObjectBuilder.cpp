#include "RaytracingObjectBuilder.h"

using namespace DirectX;

Graphics::Assets::RaytracingObjectBuilder::RaytracingObjectBuilder()
	: raytracingLibraryDesc{}
{
	SetBuffer(0u, 0u);
}

Graphics::Assets::RaytracingObjectBuilder::~RaytracingObjectBuilder()
{
	Reset();
}

void Graphics::Assets::RaytracingObjectBuilder::AddAccelerationStructure(const AccelerationStructureDesc& desc)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = desc.isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	geometryDesc.Triangles.VertexBuffer.StartAddress = desc.vertexBufferAddress;
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = desc.vertexStride;
	geometryDesc.Triangles.VertexCount = desc.verticesNumber;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = desc.indexBufferAddress;
	geometryDesc.Triangles.IndexCount = desc.indicesNumber;
	geometryDesc.Triangles.IndexFormat = desc.indexStride == 2u ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	geometryDesc.Triangles.Transform3x4 = desc.transformsAddress;

	geometryDescs.push_back(geometryDesc);
}

void Graphics::Assets::RaytracingObjectBuilder::AddAccelerationStructure(const AccelerationStructureAABBDesc& desc)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	geometryDesc.Flags = desc.isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	geometryDesc.AABBs.AABBCount = desc.instancesNumber;
	geometryDesc.AABBs.AABBs.StartAddress = desc.aabbBufferAddress;
	geometryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

	aabbGeometryDescs.push_back(geometryDesc);
}

void Graphics::Assets::RaytracingObjectBuilder::SetRootConstants(uint32_t registerIndex, uint32_t constantsNumber)
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameter.Constants.RegisterSpace = 0;
	rootParameter.Constants.ShaderRegister = registerIndex;
	rootParameter.Constants.Num32BitValues = constantsNumber;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	rootConstantIndices.insert({ registerIndex, rootParameterIndex });

	auto signatureVariant = static_cast<uint64_t>(RootSignatureUsing::GLOBAL);

	if (!rootParameters.contains(signatureVariant))
		rootParameters.insert({ signatureVariant, {} });

	rootParameters[signatureVariant].push_back(std::move(rootParameter));
}

void Graphics::Assets::RaytracingObjectBuilder::SetConstantBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_CBV, gpuAddress,
		constantBufferSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		gpuDescriptor, textureSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_SRV, gpuAddress,
		bufferSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetRWTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		gpuDescriptor, textureSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetRWBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_UAV, gpuAddress,
		rwBufferSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetSampler(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
{
	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		gpuDescriptor, textureSlots, { 0u, RootSignatureUsing::GLOBAL });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalRootConstants(uint32_t registerIndex,
	uint32_t constantsNumber, void* constants, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameter.Constants.RegisterSpace = 0;
	rootParameter.Constants.ShaderRegister = registerIndex;
	rootParameter.Constants.Num32BitValues = constantsNumber;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	auto rootParameterIndex = static_cast<uint32_t>(rootParameters.size());

	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!rootParameters.contains(signatureVariant.GetHash()))
		rootParameters.insert({ signatureVariant.GetHash(), {} });

	rootParameters[signatureVariant.GetHash()].push_back(std::move(rootParameter));

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localRootConstants = localRootData[signatureVariant.GetHash()].rootConstants;
	localRootConstants.insert({ registerIndex, {} });
	auto& localRootConstantsData = localRootConstants[registerIndex];
	localRootConstantsData.rootParameterIndex = rootParameterIndex;
	localRootConstantsData.data.resize(constantsNumber);

	auto startAddress = reinterpret_cast<uint32_t*>(constants);
	auto endAddress = startAddress + constantsNumber;
	std::copy(startAddress, endAddress, localRootConstantsData.data.data());

	localRootParametersOrder.push_back({ registerIndex, variantIndex, RootParameterOrder::ROOT_CONSTANTS });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalConstantBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localConstantBufferSlots = localRootData[signatureVariant.GetHash()].constantBufferSlots;
	auto slotIndex = static_cast<uint32_t>(localConstantBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_CBV, gpuAddress,
		localConstantBufferSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::CONSTANT_BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localTextureSlots = localRootData[signatureVariant.GetHash()].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localBufferSlots = localRootData[signatureVariant.GetHash()].bufferSlots;
	auto slotIndex = static_cast<uint32_t>(localBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_SRV, gpuAddress,
		localBufferSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalRWTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localTextureSlots = localRootData[signatureVariant.GetHash()].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalRWBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localRWBufferSlots = localRootData[signatureVariant.GetHash()].rwBufferSlots;
	auto slotIndex = static_cast<uint32_t>(localRWBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_UAV, gpuAddress,
		localRWBufferSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::RW_BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalSampler(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	if (!localRootData.contains(signatureVariant.GetHash()))
		localRootData.insert({ signatureVariant.GetHash(), {} });

	auto& localTextureSlots = localRootData[signatureVariant.GetHash()].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
}

void Graphics::Assets::RaytracingObjectBuilder::SetShader(const RaytracingLibraryDesc& desc)
{
	raytracingLibraryDesc = desc;
}

Graphics::Assets::RaytracingObject* Graphics::Assets::RaytracingObjectBuilder::Compose(ID3D12Device5* device,
	ID3D12GraphicsCommandList5* commandList, BufferManager* bufferManager)
{
	auto rootSignatureFlags = SetRootSignatureFlags();

	ID3D12RootSignature* globalRootSignatures = nullptr;
	std::map<uint64_t, ID3D12RootSignature*> localRootSignatures;

	for (auto& rootParameterArray : rootParameters)
	{
		if (RootSignatureVariant::Compare(rootParameterArray.first, RootSignatureUsing::GLOBAL))
		{
			globalRootSignatures = CreateRootSignature(device, rootSignatureFlags,
				rootParameterArray.second.data(), static_cast<uint32_t>(rootParameterArray.second.size()));
		}
		else
		{
			auto rootSignature = CreateRootSignature(device, rootSignatureFlags,
				rootParameterArray.second.data(), static_cast<uint32_t>(rootParameterArray.second.size()));
			localRootSignatures.insert({ rootParameterArray.first, rootSignature });
		}
	}

	if (globalRootSignatures == nullptr)
		globalRootSignatures = CreateRootSignature(device, rootSignatureFlags, nullptr, 0u);

	auto stateObject = CreateStateObject(device, globalRootSignatures, localRootSignatures);
	auto ratracingObjectDesc = SetRaytracingObjectDesc(device, commandList, bufferManager, stateObject,
		globalRootSignatures, localRootSignatures);

	auto newRaytracingObject = new RaytracingObject(ratracingObjectDesc);

	Reset();

	return newRaytracingObject;
}

void Graphics::Assets::RaytracingObjectBuilder::Reset()
{
	raytracingLibraryDesc = {};

	rootConstantIndices.clear();
	constantBufferSlots.clear();
	bufferSlots.clear();
	rwBufferSlots.clear();
	textureSlots.clear();

	for (auto& rootParameterArray : rootParameters)
		for (auto& parameter : rootParameterArray.second)
			if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
				delete parameter.DescriptorTable.pDescriptorRanges;

	rootParameters.clear();
	libraryExports.clear();
	geometryDescs.clear();

	SetBuffer(0u, 0u);
}

void Graphics::Assets::RaytracingObjectBuilder::SetDescriptorParameter(uint32_t registerIndex,
	D3D12_ROOT_PARAMETER_TYPE parameterType, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
	std::vector<DescriptorSlot>& slots, RootSignatureVariant signatureVariant)
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

	if (!rootParameters.contains(signatureVariant.GetHash()))
		rootParameters.insert({ signatureVariant.GetHash(), {} });

	rootParameters[signatureVariant.GetHash()].push_back(std::move(rootParameter));
}

void Graphics::Assets::RaytracingObjectBuilder::SetDescriptorTableParameter(uint32_t registerIndex,
	D3D12_DESCRIPTOR_RANGE_TYPE rangeType, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
	std::vector<DescriptorTableSlot>& slots, RootSignatureVariant signatureVariant)
{
	DescriptorTableSlot slot{};
	slot.rootParameterIndex = static_cast<uint32_t>(rootParameters.size());
	slot.gpuDescriptor = gpuDescriptor;

	slots.push_back(std::move(slot));

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

	if (!rootParameters.contains(signatureVariant.GetHash()))
		rootParameters.insert({ signatureVariant.GetHash(), {} });

	rootParameters[signatureVariant.GetHash()].push_back(std::move(rootParameter));
}

D3D12_ROOT_SIGNATURE_FLAGS Graphics::Assets::RaytracingObjectBuilder::SetRootSignatureFlags()
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

Graphics::Assets::RaytracingObjectDesc Graphics::Assets::RaytracingObjectBuilder::SetRaytracingObjectDesc(ID3D12Device5* device,
	ID3D12GraphicsCommandList5* commandList, BufferManager* bufferManager, ID3D12StateObject* stateObject,
	ID3D12RootSignature* globalRootSignature, const std::map<uint64_t, ID3D12RootSignature*>& localRootSignatures)
{
	ID3D12StateObjectProperties* stateObjectProperties;
	stateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties));

	auto rayGenerationIdentifier = stateObjectProperties->GetShaderIdentifier(raytracingLibraryDesc.rayGenerationShaderName);
	
	uint64_t rayGenerationSize = 0u;
	uint64_t missStride = 0u;
	uint64_t missSize = 0u;
	uint64_t hitStride = 0u;
	uint64_t hitSize = 0u;

	auto shaderTablesSize = CalculateShaderTablesSize(rayGenerationSize, missStride, missSize, hitStride, hitSize);
	auto bufferAllocation = bufferManager->Allocate(device, shaderTablesSize, BufferAllocationType::SHADER_TABLES);

	auto startAddress = bufferAllocation.gpuAddress;
	auto destAddress = bufferAllocation.cpuAddress;

	D3D12_DISPATCH_RAYS_DESC dipatchDesc{};

	dipatchDesc.RayGenerationShaderRecord.StartAddress = startAddress;
	dipatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSize;

	FillShaderTable({ 0u, RootSignatureUsing::RAY_GENERATION }, reinterpret_cast<uint8_t*>(rayGenerationIdentifier),
		destAddress);

	startAddress += rayGenerationSize;
	destAddress += rayGenerationSize;

	dipatchDesc.MissShaderTable.StartAddress = startAddress;
	dipatchDesc.MissShaderTable.StrideInBytes = missStride;
	dipatchDesc.MissShaderTable.SizeInBytes = missSize;

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.missShaderNames.size(); variantIndex++)
	{
		auto& missShaderName = raytracingLibraryDesc.missShaderNames[variantIndex];
		auto missIdentifier = stateObjectProperties->GetShaderIdentifier(missShaderName);

		FillShaderTable({ variantIndex, RootSignatureUsing::MISS }, reinterpret_cast<uint8_t*>(missIdentifier),
			destAddress);

		destAddress += missStride;
	}

	startAddress += missSize;
	destAddress = bufferAllocation.cpuAddress + rayGenerationSize + missSize;

	dipatchDesc.HitGroupTable.StartAddress = startAddress;
	dipatchDesc.HitGroupTable.StrideInBytes = hitStride;
	dipatchDesc.HitGroupTable.SizeInBytes = hitSize;

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.triangleHitGroups.size(); variantIndex++)
	{
		auto& hitGroup = raytracingLibraryDesc.triangleHitGroups[variantIndex];
		auto hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroup.hitGroupName);

		FillShaderTable({ variantIndex, RootSignatureUsing::TRIANGLE_HIT_GROUP },
			reinterpret_cast<uint8_t*>(hitGroupIdentifier), destAddress);

		destAddress += hitStride;
	}

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.aabbHitGroups.size(); variantIndex++)
	{
		auto& hitGroup = raytracingLibraryDesc.aabbHitGroups[variantIndex];
		auto hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroup.hitGroupName);

		FillShaderTable({ variantIndex, RootSignatureUsing::AABB_HIT_GROUP },
			reinterpret_cast<uint8_t*>(hitGroupIdentifier), destAddress);

		destAddress += hitStride;
	}

	dipatchDesc.Width = 1u;
	dipatchDesc.Height = 1u;
	dipatchDesc.Depth = 1u;

	std::vector<ID3D12RootSignature*> rootSignatures;
	rootSignatures.reserve(localRootSignatures.size());

	for (auto& rootSignature : localRootSignatures)
		rootSignatures.push_back(rootSignature.second);

	BufferAllocation bottomLevelStructure{};
	BufferAllocation bottomLevelStructureAABB{};
	
	if (!geometryDescs.empty())
		CreateBottomLevelAccelerationStructure(device, commandList, bufferManager, geometryDescs,
			bottomLevelStructure);

	if (!aabbGeometryDescs.empty())
		CreateBottomLevelAccelerationStructure(device, commandList, bufferManager, aabbGeometryDescs,
			bottomLevelStructureAABB);
	
	if (bottomLevelStructure.resource != nullptr)
		bottomLevelStructure.resource->UAVBarrier(commandList);

	if (bottomLevelStructureAABB.resource != nullptr)
		bottomLevelStructureAABB.resource->UAVBarrier(commandList);

	BufferAllocation topLevelStructure{};

	CreateTopLevelAccelerationStructure(device, commandList, bufferManager, bottomLevelStructure,
		bottomLevelStructureAABB, topLevelStructure);

	bufferSlots[0].gpuAddress = topLevelStructure.gpuAddress;

	RaytracingObjectDesc desc
	{
		globalRootSignature,
		rootSignatures,
		stateObject,
		rootConstantIndices,
		constantBufferSlots,
		bufferSlots,
		rwBufferSlots,
		textureSlots,
		dipatchDesc,
		bufferAllocation,
		bottomLevelStructure,
		bottomLevelStructureAABB,
		topLevelStructure
	};

	return desc;
}

ID3D12RootSignature* Graphics::Assets::RaytracingObjectBuilder::CreateRootSignature(ID3D12Device5* device,
	D3D12_ROOT_SIGNATURE_FLAGS flags, D3D12_ROOT_PARAMETER* rootParameterArray, uint32_t rootParameterNumber)
{
	ID3D12RootSignature* rootSignature = nullptr;
	
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = rootParameterNumber;

	if (rootParameterNumber > 0)
		rootSignatureDesc.pParameters = rootParameterArray;

	rootSignatureDesc.Flags = flags;

	ID3DBlob* signature;
	ID3DBlob* error;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);

	if (error != nullptr)
	{
		OutputDebugStringA(reinterpret_cast<char*>(error->GetBufferPointer()));

		error->Release();
	}

	device->CreateRootSignature(0u, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	signature->Release();

	return rootSignature;
}

ID3D12StateObject* Graphics::Assets::RaytracingObjectBuilder::CreateStateObject(ID3D12Device5* device,
	ID3D12RootSignature* globalRootSignature, const std::map<uint64_t, ID3D12RootSignature*>& localRootSignatures)
{
	ID3D12StateObject* stateObject = nullptr;

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.push_back(CreateDXILLibrarySubobject());

	for (auto& hitGroup : raytracingLibraryDesc.triangleHitGroups)
		subobjects.push_back(CreateHitGroupSubobject(hitGroup, true));

	for (auto& hitGroup : raytracingLibraryDesc.aabbHitGroups)
		subobjects.push_back(CreateHitGroupSubobject(hitGroup, false));

	subobjects.push_back(CreateRaytracingConfigSubobject());

	for (auto& localRootSignature : localRootSignatures)
	{
		D3D12_STATE_SUBOBJECT localRootSubobject;
		D3D12_STATE_SUBOBJECT assotiationSubobject;
		CreateLocalRootSignatureSubobject(localRootSignature.second,
			RootSignatureVariant::GetVariantFromHash(localRootSignature.first),
			localRootSubobject, assotiationSubobject);

		subobjects.push_back(localRootSubobject);
		subobjects.push_back(assotiationSubobject);
	}

	subobjects.push_back(CreateGlobalRootSignatureSubobject(globalRootSignature));
	subobjects.push_back(CreatePipelineConfigSubobject());

	D3D12_STATE_OBJECT_DESC stateObjectDesc{};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = static_cast<uint32_t>(subobjects.size());
	stateObjectDesc.pSubobjects = subobjects.data();

	device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject));

	return stateObject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateDXILLibrarySubobject()
{
	AddLibraryExport(raytracingLibraryDesc.rayGenerationShaderName);

	for (auto& missShaderName : raytracingLibraryDesc.missShaderNames)
		AddLibraryExport(missShaderName);

	for (auto& hitGroup : raytracingLibraryDesc.triangleHitGroups)
	{
		if (hitGroup.anyHitShaderName != L"")
			AddLibraryExport(hitGroup.anyHitShaderName);

		if (hitGroup.closestHitShaderName != L"")
			AddLibraryExport(hitGroup.closestHitShaderName);

		if (hitGroup.intersectionShaderName != L"")
			AddLibraryExport(hitGroup.intersectionShaderName);
	}

	for (auto& hitGroup : raytracingLibraryDesc.aabbHitGroups)
	{
		if (hitGroup.anyHitShaderName != L"")
			AddLibraryExport(hitGroup.anyHitShaderName);

		if (hitGroup.closestHitShaderName != L"")
			AddLibraryExport(hitGroup.closestHitShaderName);

		if (hitGroup.intersectionShaderName != L"")
			AddLibraryExport(hitGroup.intersectionShaderName);
	}

	D3D12_DXIL_LIBRARY_DESC libraryDesc{};
	libraryDesc.DXILLibrary = raytracingLibraryDesc.dxilLibrary;
	libraryDesc.NumExports = static_cast<uint32_t>(libraryExports.size());
	libraryDesc.pExports = libraryExports.data();
	
	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobject.pDesc = &libraryDesc;

	return subobject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateHitGroupSubobject(const RaytracingHitGroup& hitGroup,
	bool isTriangle)
{
	D3D12_HIT_GROUP_DESC hitGroupDesc{};
	hitGroupDesc.Type = isTriangle ? D3D12_HIT_GROUP_TYPE_TRIANGLES : D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
	hitGroupDesc.HitGroupExport = hitGroup.hitGroupName;

	if (hitGroup.anyHitShaderName != L"")
		hitGroupDesc.AnyHitShaderImport = hitGroup.anyHitShaderName;

	if (hitGroup.closestHitShaderName != L"")
		hitGroupDesc.ClosestHitShaderImport = hitGroup.closestHitShaderName;

	if (hitGroup.intersectionShaderName != L"")
		hitGroupDesc.IntersectionShaderImport = hitGroup.intersectionShaderName;

	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobject.pDesc = &hitGroupDesc;

	return subobject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateRaytracingConfigSubobject()
{
	D3D12_RAYTRACING_SHADER_CONFIG raytracingConfig{};
	raytracingConfig.MaxPayloadSizeInBytes = raytracingLibraryDesc.payloadStride;
	raytracingConfig.MaxAttributeSizeInBytes = raytracingLibraryDesc.attributeStride;

	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobject.pDesc = &raytracingConfig;

	return subobject;
}

void Graphics::Assets::RaytracingObjectBuilder::CreateLocalRootSignatureSubobject(ID3D12RootSignature* rootSignature,
	RootSignatureVariant signatureVariant, D3D12_STATE_SUBOBJECT& localRootSubobject,
	D3D12_STATE_SUBOBJECT& assotiationSubobject)
{
	D3D12_LOCAL_ROOT_SIGNATURE localSignature{};
	localSignature.pLocalRootSignature = rootSignature;

	localRootSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	localRootSubobject.pDesc = &localSignature;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assotiation{};
	assotiation.pSubobjectToAssociate = &localRootSubobject;
	assotiation.NumExports = 1u;

	if (signatureVariant.signatureUsing == RootSignatureUsing::RAY_GENERATION)
		assotiation.pExports = &raytracingLibraryDesc.rayGenerationShaderName;
	else if (signatureVariant.signatureUsing == RootSignatureUsing::MISS)
		assotiation.pExports = &raytracingLibraryDesc.missShaderNames[signatureVariant.variantIndex];
	else if(signatureVariant.signatureUsing == RootSignatureUsing::TRIANGLE_HIT_GROUP)
		assotiation.pExports = &raytracingLibraryDesc.triangleHitGroups[signatureVariant.variantIndex].hitGroupName;
	else if (signatureVariant.signatureUsing == RootSignatureUsing::AABB_HIT_GROUP)
		assotiation.pExports = &raytracingLibraryDesc.aabbHitGroups[signatureVariant.variantIndex].hitGroupName;

	assotiationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	assotiationSubobject.pDesc = &assotiation;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateGlobalRootSignatureSubobject(ID3D12RootSignature* rootSignature)
{
	D3D12_GLOBAL_ROOT_SIGNATURE globalSignature{};
	globalSignature.pGlobalRootSignature = rootSignature;

	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobject.pDesc = &globalSignature;

	return subobject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreatePipelineConfigSubobject()
{
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{};
	pipelineConfig.MaxTraceRecursionDepth = raytracingLibraryDesc.maxRecursionLevel;

	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobject.pDesc = &pipelineConfig;

	return subobject;
}

void Graphics::Assets::RaytracingObjectBuilder::AddLibraryExport(const LPCWSTR& name)
{
	if (name != L"")
	{
		D3D12_EXPORT_DESC libraryExport
		{
			name,
			nullptr,
			D3D12_EXPORT_FLAG_NONE
		};

		libraryExports.push_back(libraryExport);
	}
}

uint64_t Graphics::Assets::RaytracingObjectBuilder::CalculateShaderTablesSize(uint64_t& rayGenerationSize,
	uint64_t& missStride, uint64_t& missSize, uint64_t& hitStride, uint64_t& hitSize)
{
	rayGenerationSize = 0u;
	missStride = 0u;
	missSize = 0u;
	hitStride = 0u;
	hitSize = 0u;
	
	for (auto& localRootDataElement : localRootData)
	{
		if (RootSignatureVariant::Compare(localRootDataElement.first, RootSignatureUsing::GLOBAL))
			continue;

		uint64_t recordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		auto& rootData = localRootDataElement.second;

		for (auto& rootConstant : rootData.rootConstants)
			recordSize += rootConstant.second.data.size() * sizeof(uint32_t);

		recordSize += rootData.constantBufferSlots.size() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		recordSize += rootData.bufferSlots.size() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		recordSize += rootData.rwBufferSlots.size() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		recordSize += rootData.textureSlots.size() * sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		recordSize = AlignSize(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		if (RootSignatureVariant::Compare(localRootDataElement.first, RootSignatureUsing::RAY_GENERATION))
			rayGenerationSize = recordSize;
		else if (RootSignatureVariant::Compare(localRootDataElement.first, RootSignatureUsing::MISS))
		{
			missStride = missStride < recordSize ? recordSize : missStride;
			missSize += recordSize;
		}
		else if (RootSignatureVariant::Compare(localRootDataElement.first, RootSignatureUsing::TRIANGLE_HIT_GROUP) ||
			RootSignatureVariant::Compare(localRootDataElement.first, RootSignatureUsing::AABB_HIT_GROUP))
		{
			hitStride = hitStride < recordSize ? recordSize : hitStride;
			hitSize += recordSize;
		}
	}

	rayGenerationSize = AlignSize(rayGenerationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	missSize = AlignSize(missSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	hitSize = AlignSize(hitSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	return rayGenerationSize + missSize + hitSize;
}

void Graphics::Assets::RaytracingObjectBuilder::FillShaderTable(RootSignatureVariant signatureVariant,
	const uint8_t* shaderIdentifier, uint8_t* destBuffer)
{
	auto destAddress = destBuffer;
	std::copy(shaderIdentifier, shaderIdentifier + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, destAddress);
	destAddress += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	auto& rootData = localRootData[signatureVariant.GetHash()];

	for (auto& order : localRootParametersOrder)
	{
		if (order.order == RootParameterOrder::ROOT_CONSTANTS)
		{
			auto& rootConstants = rootData.rootConstants[order.index];
			
			auto startAddress = rootConstants.data.data();
			auto endAddress = startAddress + rootConstants.data.size();
			std::copy(startAddress, endAddress, reinterpret_cast<uint32_t*>(destAddress));

			destAddress += rootConstants.data.size() * sizeof(uint32_t);
		}
		else if (order.order == RootParameterOrder::CONSTANT_BUFFER)
		{
			auto& slot = rootData.constantBufferSlots[order.index];
			*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(destAddress) = slot.gpuAddress;
			destAddress += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}
		else if (order.order == RootParameterOrder::TEXTURE)
		{
			auto& slot = rootData.textureSlots[order.index];
			*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(destAddress) = slot.gpuDescriptor;
			destAddress += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
		}
		else if (order.order == RootParameterOrder::BUFFER)
		{
			auto& slot = rootData.bufferSlots[order.index];
			*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(destAddress) = slot.gpuAddress;
			destAddress += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}
		else if (order.order == RootParameterOrder::RW_BUFFER)
		{
			auto& slot = rootData.rwBufferSlots[order.index];
			*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(destAddress) = slot.gpuAddress;
			destAddress += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}
	}
}

void Graphics::Assets::RaytracingObjectBuilder::CreateBottomLevelAccelerationStructure(ID3D12Device5* device,
	ID3D12GraphicsCommandList5* commandList, BufferManager* bufferManager,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescArray,
	BufferAllocation& bottomLevelStructure)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc{};
	desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.Inputs.NumDescs = static_cast<uint32_t>(geometryDescArray.size());
	desc.Inputs.pGeometryDescs = geometryDescArray.data();
	
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&desc.Inputs, &prebuildInfo);

	auto scratchBuffer = bufferManager->Allocate(device, prebuildInfo.ScratchDataSizeInBytes,
		BufferAllocationType::UNORDERED_ACCESS_TEMP);

	bottomLevelStructure = bufferManager->Allocate(device, prebuildInfo.ResultDataMaxSizeInBytes,
		BufferAllocationType::ACCELERATION_STRUCTURE_BOTTOM_LEVEL);

	desc.ScratchAccelerationStructureData = scratchBuffer.gpuAddress;
	desc.DestAccelerationStructureData = bottomLevelStructure.gpuAddress;

	commandList->BuildRaytracingAccelerationStructure(&desc, 0u, nullptr);
}

void Graphics::Assets::RaytracingObjectBuilder::CreateTopLevelAccelerationStructure(ID3D12Device5* device,
	ID3D12GraphicsCommandList5* commandList, BufferManager* bufferManager,
	const BufferAllocation& bottomLevelStructure, const BufferAllocation& bottomLevelStructureAABB,
	BufferAllocation& topLevelStructure)
{
	bool hasStruct = bottomLevelStructure.resource != nullptr;
	bool hasStructAABB = bottomLevelStructureAABB.resource != nullptr;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc{};
	desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	desc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.Inputs.NumDescs = hasStruct && hasStructAABB ? 2u : 1u;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&desc.Inputs, &prebuildInfo);

	auto scratchBuffer = bufferManager->Allocate(device, prebuildInfo.ScratchDataSizeInBytes,
		BufferAllocationType::UNORDERED_ACCESS_TEMP);

	desc.ScratchAccelerationStructureData = scratchBuffer.gpuAddress;

	topLevelStructure = bufferManager->Allocate(device, prebuildInfo.ResultDataMaxSizeInBytes,
		BufferAllocationType::ACCELERATION_STRUCTURE_BOTTOM_LEVEL);

	desc.DestAccelerationStructureData = topLevelStructure.gpuAddress;

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	
	if (hasStruct)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};

		instanceDesc.InstanceMask = 1u;
		instanceDesc.InstanceContributionToHitGroupIndex = 0u;
		instanceDesc.AccelerationStructure = bottomLevelStructure.gpuAddress;

		XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), XMMatrixIdentity());

		instanceDescs.push_back(instanceDesc);
	}

	if (hasStructAABB)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};

		instanceDesc.InstanceMask = 1u;
		instanceDesc.InstanceContributionToHitGroupIndex = raytracingLibraryDesc.triangleHitGroups.size();
		instanceDesc.AccelerationStructure = bottomLevelStructureAABB.gpuAddress;

		XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), XMMatrixIdentity());

		instanceDescs.push_back(instanceDesc);
	}

	auto instanceDescBufferSize = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	auto instanceDescBuffer = bufferManager->Allocate(device, instanceDescBufferSize, BufferAllocationType::UPLOAD);
	
	auto startAddress = reinterpret_cast<const uint8_t*>(instanceDescs.data());
	auto endAddress = startAddress + instanceDescBufferSize;
	std::copy(startAddress, endAddress, instanceDescBuffer.cpuAddress);

	desc.Inputs.InstanceDescs = instanceDescBuffer.gpuAddress;
	
	commandList->BuildRaytracingAccelerationStructure(&desc, 0u, nullptr);
}

uint64_t Graphics::Assets::RaytracingObjectBuilder::AlignSize(uint64_t size, uint64_t alignment)
{
	return (size + alignment - 1u) & ~(alignment - 1u);
}
