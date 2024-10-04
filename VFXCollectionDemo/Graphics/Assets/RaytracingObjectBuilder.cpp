#include "RaytracingObjectBuilder.h"
#include "Raytracing/RaytracingShaderTable.h"

using namespace DirectX;
using namespace Graphics::Assets::Raytracing;

Graphics::Assets::RaytracingObjectBuilder::RaytracingObjectBuilder()
	: raytracingLibraryDesc{}, libraryDesc{}, raytracingConfig{},
	assotiation{}, globalSignature{}, pipelineConfig{}, rootSignatureIndexCounter{}
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
	geometryDesc.Flags = desc.isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
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
	geometryDesc.Flags = desc.isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
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

	auto rootParameterIndex = rootSignatureIndexCounter++;
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

	auto rootParameterIndex = rootSignatureIndexCounter++;

	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!rootParameters.contains(hash))
		rootParameters.insert({ hash, {} });

	rootParameters[hash].push_back(std::move(rootParameter));

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localRootConstants = localRootData[hash].rootConstants;
	localRootConstants.insert({ registerIndex, {} });
	auto& localRootConstantsData = localRootConstants[registerIndex];
	localRootConstantsData.rootParameterIndex = rootParameterIndex;
	localRootConstantsData.data.resize(constantsNumber);

	auto startAddress = reinterpret_cast<uint32_t*>(constants);
	auto endAddress = startAddress + constantsNumber;
	std::copy(startAddress, endAddress, localRootConstantsData.data.data());

	localRootData[hash].localRootParametersOrder.push_back({ registerIndex, variantIndex, RootParameterOrder::ROOT_CONSTANTS });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalConstantBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localConstantBufferSlots = localRootData[hash].constantBufferSlots;
	auto slotIndex = static_cast<uint32_t>(localConstantBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_CBV, gpuAddress,
		localConstantBufferSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::CONSTANT_BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localTextureSlots = localRootData[hash].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localBufferSlots = localRootData[hash].bufferSlots;
	auto slotIndex = static_cast<uint32_t>(localBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_SRV, gpuAddress,
		localBufferSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalRWTexture(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localTextureSlots = localRootData[hash].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalRWBuffer(uint32_t registerIndex,
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localRWBufferSlots = localRootData[hash].rwBufferSlots;
	auto slotIndex = static_cast<uint32_t>(localRWBufferSlots.size());

	SetDescriptorParameter(registerIndex, D3D12_ROOT_PARAMETER_TYPE_UAV, gpuAddress,
		localRWBufferSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::RW_BUFFER });
}

void Graphics::Assets::RaytracingObjectBuilder::SetLocalSampler(uint32_t registerIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, RootSignatureUsing signatureUsing, uint32_t variantIndex)
{
	RootSignatureVariant signatureVariant{};
	signatureVariant.signatureUsing = signatureUsing;
	signatureVariant.variantIndex = variantIndex;

	auto hash = signatureVariant.GetHash();

	if (!localRootData.contains(hash))
		localRootData.insert({ hash, {} });

	auto& localTextureSlots = localRootData[hash].textureSlots;
	auto slotIndex = static_cast<uint32_t>(localTextureSlots.size());

	SetDescriptorTableParameter(registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		gpuDescriptor, localTextureSlots, signatureVariant);

	localRootData[hash].localRootParametersOrder.push_back({ slotIndex, variantIndex, RootParameterOrder::TEXTURE });
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
	hitGroupDescs.clear();
	localSignatures.clear();

	SetBuffer(0u, 0u);
}

void Graphics::Assets::RaytracingObjectBuilder::SetDescriptorParameter(uint32_t registerIndex,
	D3D12_ROOT_PARAMETER_TYPE parameterType, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
	std::vector<DescriptorSlot>& slots, RootSignatureVariant signatureVariant)
{
	DescriptorSlot slot{};
	slot.shaderRegisterIndex = registerIndex;
	slot.rootParameterIndex = rootSignatureIndexCounter++;
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
	slot.rootParameterIndex = rootSignatureIndexCounter++;
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

	RaytracingShaderTable shaderTable{};

	auto rayGenerationIdentifier = stateObjectProperties->GetShaderIdentifier(raytracingLibraryDesc.rayGenerationShaderName);
	auto rayGenerationShaderRecord = CreateShaderRecord({ 0u, RootSignatureUsing::RAY_GENERATION }, rayGenerationIdentifier);
	shaderTable.AddRecord(std::move(rayGenerationShaderRecord), RaytracingShaderRecordKind::RAY_GENERATION_SHADER_RECORD);

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.missShaderNames.size(); variantIndex++)
	{
		auto& missShaderName = raytracingLibraryDesc.missShaderNames[variantIndex];
		auto missIdentifier = stateObjectProperties->GetShaderIdentifier(missShaderName);

		auto missShaderRecord = CreateShaderRecord({ variantIndex, RootSignatureUsing::MISS }, missIdentifier);
		shaderTable.AddRecord(std::move(missShaderRecord), RaytracingShaderRecordKind::MISS_SHADER_RECORD);
	}

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.triangleHitGroups.size(); variantIndex++)
	{
		void* hitGroupIdentifier = nullptr;
		auto& hitGroup = raytracingLibraryDesc.triangleHitGroups[variantIndex];

		auto& hitGroupName = hitGroup.hitGroupName;
		hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroupName);
		
		auto hitGroupRecord = CreateShaderRecord({ variantIndex, RootSignatureUsing::TRIANGLE_HIT_GROUP }, hitGroupIdentifier);
		shaderTable.AddRecord(std::move(hitGroupRecord), RaytracingShaderRecordKind::HIT_GROUP_SHADER_RECORD);
	}

	for (uint32_t variantIndex = 0u; variantIndex < raytracingLibraryDesc.aabbHitGroups.size(); variantIndex++)
	{
		void* hitGroupIdentifier = nullptr;
		auto& hitGroup = raytracingLibraryDesc.aabbHitGroups[variantIndex];

		auto& hitGroupName = hitGroup.hitGroupName;
		hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroupName);
		
		auto hitGroupRecord = CreateShaderRecord({ variantIndex, RootSignatureUsing::AABB_HIT_GROUP }, hitGroupIdentifier);
		shaderTable.AddRecord(std::move(hitGroupRecord), RaytracingShaderRecordKind::HIT_GROUP_SHADER_RECORD);
	}

	stateObjectProperties->Release();

	auto shaderTableRawBuffer = shaderTable.Compose();

	auto bufferAllocation = bufferManager->Allocate(device, shaderTableRawBuffer.size(), BufferAllocationType::SHADER_TABLES);

	auto startAddress = shaderTableRawBuffer.data();
	auto endAddress = startAddress + shaderTableRawBuffer.size();
	auto destAddress = bufferAllocation.cpuAddress;

	std::copy(startAddress, endAddress, destAddress);

	bufferAllocation.resource->Unmap();
	
	D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = bufferAllocation.gpuAddress;
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = shaderTable.GetRayGenerationShaderRecordSize();
	dispatchDesc.MissShaderTable.StartAddress = bufferAllocation.gpuAddress + shaderTable.GetRayGenerationShaderTableSize();
	dispatchDesc.MissShaderTable.StrideInBytes = shaderTable.GetMissShaderRecordSize();
	dispatchDesc.MissShaderTable.SizeInBytes = shaderTable.GetMissTableSize();
	dispatchDesc.HitGroupTable.StartAddress = dispatchDesc.MissShaderTable.StartAddress + shaderTable.GetMissTableSize();
	dispatchDesc.HitGroupTable.StrideInBytes = shaderTable.GetHitGroupShaderRecordSize();
	dispatchDesc.HitGroupTable.SizeInBytes = shaderTable.GetHitGroupTableSize();
	dispatchDesc.Width = 1u;
	dispatchDesc.Height = 1u;
	dispatchDesc.Depth = 1u;

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
		dispatchDesc,
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

	device->CreateRootSignature(1u, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	signature->Release();

	return rootSignature;
}

ID3D12StateObject* Graphics::Assets::RaytracingObjectBuilder::CreateStateObject(ID3D12Device5* device,
	ID3D12RootSignature* globalRootSignature, const std::map<uint64_t, ID3D12RootSignature*>& localRootSignatures)
{
	ID3D12StateObject* stateObject = nullptr;

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.push_back(CreateDXILLibrarySubobject());

	auto hitGroupsTotalNumber = raytracingLibraryDesc.triangleHitGroups.size() + raytracingLibraryDesc.aabbHitGroups.size();
	hitGroupDescs.reserve(hitGroupsTotalNumber);

	for (auto& hitGroup : raytracingLibraryDesc.triangleHitGroups)
		subobjects.push_back(CreateHitGroupSubobject(hitGroup, true));

	for (auto& hitGroup : raytracingLibraryDesc.aabbHitGroups)
		subobjects.push_back(CreateHitGroupSubobject(hitGroup, false));

	subobjects.push_back(CreateRaytracingConfigSubobject());

	localSignatures.reserve(localRootSignatures.size());

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

	auto hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject));

	return stateObject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateDXILLibrarySubobject()
{
	AddLibraryExport(raytracingLibraryDesc.rayGenerationShaderName);

	for (auto& hitGroup : raytracingLibraryDesc.triangleHitGroups)
	{
		if (hitGroup.anyHitShaderName != nullptr && hitGroup.anyHitShaderName != L"")
			AddLibraryExport(hitGroup.anyHitShaderName);

		if (hitGroup.closestHitShaderName != nullptr && hitGroup.closestHitShaderName != L"")
			AddLibraryExport(hitGroup.closestHitShaderName);
	}

	for (auto& hitGroup : raytracingLibraryDesc.aabbHitGroups)
	{
		if (hitGroup.anyHitShaderName != nullptr && hitGroup.anyHitShaderName != L"")
			AddLibraryExport(hitGroup.anyHitShaderName);

		if (hitGroup.closestHitShaderName != nullptr && hitGroup.closestHitShaderName != L"")
			AddLibraryExport(hitGroup.closestHitShaderName);

		if (hitGroup.intersectionShaderName != nullptr && hitGroup.intersectionShaderName != L"")
			AddLibraryExport(hitGroup.intersectionShaderName);
	}

	for (auto& missShaderName : raytracingLibraryDesc.missShaderNames)
		AddLibraryExport(missShaderName);

	libraryDesc = {};
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
	hitGroupDescs.push_back({});

	auto& hitGroupDesc = hitGroupDescs[hitGroupDescs.size() - 1];
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
	subobject.pDesc = &hitGroupDescs[hitGroupDescs.size() - 1];

	return subobject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreateRaytracingConfigSubobject()
{
	raytracingConfig = {};
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
	localSignatures.push_back({});

	auto& localSignature = localSignatures[localSignatures.size() - 1];
	localSignature.pLocalRootSignature = rootSignature;

	localRootSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	localRootSubobject.pDesc = &localSignature;

	assotiation = {};
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
	globalSignature = {};
	globalSignature.pGlobalRootSignature = rootSignature;

	D3D12_STATE_SUBOBJECT subobject{};
	subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobject.pDesc = &globalSignature;

	return subobject;
}

D3D12_STATE_SUBOBJECT Graphics::Assets::RaytracingObjectBuilder::CreatePipelineConfigSubobject()
{
	pipelineConfig = {};
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

RaytracingShaderRecord Graphics::Assets::RaytracingObjectBuilder::CreateShaderRecord(RootSignatureVariant signatureVariant,
	const void* shaderIdentifier)
{
	RaytracingShaderRecord shaderRecord{};

	if (shaderIdentifier == nullptr)
	{
		std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> zeroData{};

		shaderRecord.AddData(zeroData.data(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	else
		shaderRecord.AddData(shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	auto hash = signatureVariant.GetHash();

	if (localRootData.contains(hash))
	{
		auto& rootData = localRootData[hash];

		for (auto& order : rootData.localRootParametersOrder)
		{
			if (order.order == RootParameterOrder::ROOT_CONSTANTS)
			{
				auto& rootConstants = rootData.rootConstants[order.index];
				shaderRecord.AddData(rootConstants.data.data(), rootConstants.data.size());
			}
			else if (order.order == RootParameterOrder::CONSTANT_BUFFER)
			{
				auto& slot = rootData.constantBufferSlots[order.index];
				shaderRecord.AddData(&slot.gpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
			}
			else if (order.order == RootParameterOrder::TEXTURE)
			{
				auto& slot = rootData.textureSlots[order.index];
				shaderRecord.AddData(&slot.gpuDescriptor, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
			}
			else if (order.order == RootParameterOrder::BUFFER)
			{
				auto& slot = rootData.bufferSlots[order.index];
				shaderRecord.AddData(&slot.gpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
			}
			else if (order.order == RootParameterOrder::RW_BUFFER)
			{
				auto& slot = rootData.rwBufferSlots[order.index];
				shaderRecord.AddData(&slot.gpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
			}
		}
	}

	return shaderRecord;
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
		BufferAllocationType::ACCELERATION_STRUCTURE);

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
		BufferAllocationType::ACCELERATION_STRUCTURE);

	desc.DestAccelerationStructureData = topLevelStructure.gpuAddress;

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	
	if (hasStruct)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
		instanceDesc.InstanceMask = 1u;
		instanceDesc.InstanceContributionToHitGroupIndex = 0u;
		instanceDesc.AccelerationStructure = bottomLevelStructure.gpuAddress;
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

		XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), XMMatrixIdentity());
		
		instanceDescs.push_back(instanceDesc);
	}

	if (hasStructAABB)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
		instanceDesc.InstanceMask = 1u;
		instanceDesc.InstanceContributionToHitGroupIndex = raytracingLibraryDesc.triangleHitGroups.size();
		instanceDesc.AccelerationStructure = bottomLevelStructureAABB.gpuAddress;
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

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

bool Graphics::Assets::RaytracingObjectBuilder::HitGroupIsEmpty(const RaytracingHitGroup& hitGroup)
{
	if ((hitGroup.closestHitShaderName == nullptr || hitGroup.closestHitShaderName == L"") &&
		(hitGroup.anyHitShaderName == nullptr || hitGroup.anyHitShaderName == L"") &&
		(hitGroup.intersectionShaderName == nullptr || hitGroup.intersectionShaderName == L""))
		return true;

	return false;
}
