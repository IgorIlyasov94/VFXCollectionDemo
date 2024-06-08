#include "VegetationSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/OBJLoader.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../../Graphics/Assets/GeometryUtilities.h"
#include "../../Utilities.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Common;
using namespace Graphics;
using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VegatationSystem::VegatationSystem(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, const VegetationSystemDesc& desc, const Camera* camera)
{
	_camera = camera;
	randomEngine = desc.randomEngine;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	GenerateMesh(device, commandList, resourceManager);
	CreateConstantBuffers(device, commandList, resourceManager, desc);
	CreateBuffers(device, commandList, resourceManager, desc);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager, desc);
	CreateMaterial(device, resourceManager, desc.perlinNoiseId);
}

Common::Logic::SceneEntity::VegatationSystem::~VegatationSystem()
{

}

void Common::Logic::SceneEntity::VegatationSystem::OnCompute(ID3D12GraphicsCommandList* commandList,
	float time, float deltaTime)
{

}

void Common::Logic::SceneEntity::VegatationSystem::Draw(ID3D12GraphicsCommandList* commandList,
	float time, float deltaTime)
{
	mutableConstantsBuffer->viewProjection = _camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = _camera->GetDirection();
	mutableConstantsBuffer->time = time;

	_material->Set(commandList);
	_mesh->Draw(commandList, instancesNumber);
}

void Common::Logic::SceneEntity::VegatationSystem::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Buffer>(vegetationBufferId);
	resourceManager->DeleteResource<Texture>(albedoMapId);
	resourceManager->DeleteResource<Texture>(normalMapId);

	resourceManager->DeleteResource<Shader>(vegetationVSId);
	resourceManager->DeleteResource<Shader>(vegetationPSId);

	_mesh->Release(resourceManager);
	delete _mesh;

	delete _material;
}

void Common::Logic::SceneEntity::VegatationSystem::GenerateMesh(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc vbDesc{};
	vbDesc.numElements = 4u;
	vbDesc.dataStride = sizeof(GrassVertex);
	vbDesc.data.resize(static_cast<size_t>(vbDesc.dataStride) * vbDesc.numElements);

	auto halfZero = XMConvertFloatToHalf(0.0f);
	auto halfOne = XMConvertFloatToHalf(1.0f);

	auto vertices = reinterpret_cast<GrassVertex*>(vbDesc.data.data());
	vertices[0u] = { float3(-0.5f, 0.0f, 1.0f), XMHALF2(halfZero, halfZero) };
	vertices[1u] = { float3(0.5f, 0.0f, 1.0f), XMHALF2(halfOne, halfZero) };
	vertices[2u] = { float3(0.5f, 0.0f, 0.0f), XMHALF2(halfOne, halfOne) };
	vertices[3u] = { float3(-0.5f, 0.0f, 0.0f), XMHALF2(halfZero, halfOne) };

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vbDesc);

	BufferDesc ibDesc{};
	ibDesc.numElements = 6u;
	ibDesc.dataStride = sizeof(uint16_t);
	ibDesc.data.resize(static_cast<size_t>(ibDesc.dataStride) * ibDesc.numElements);

	auto indices = reinterpret_cast<uint16_t*>(ibDesc.data.data());
	indices[0u] = 0u;
	indices[1u] = 1u;
	indices[2u] = 2u;
	indices[3u] = 2u;
	indices[4u] = 3u;
	indices[5u] = 0u;

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, ibDesc);

	MeshDesc meshDesc{};
	meshDesc.verticesNumber = vbDesc.numElements;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::TEXCOORD0;
	meshDesc.indicesNumber = ibDesc.numElements;
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	_mesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VegatationSystem::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->viewProjection = _camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = _camera->GetDirection();
	mutableConstantsBuffer->time = 0.0f;
	mutableConstantsBuffer->atlasElementSize.x = 1.0f / desc.atlasRows;
	mutableConstantsBuffer->atlasElementSize.y = 1.0f / desc.atlasColumns;
}

void Common::Logic::SceneEntity::VegatationSystem::CreateBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	uint32_t smallGrassInstanceNumber = QUADS_PER_SMALL_GRASS * desc.smallGrassNumber;
	uint32_t mediumGrassInstanceNumber = QUADS_PER_MEDIUM_GRASS * desc.mediumGrassNumber;
	uint32_t largeGrassInstanceNumber = QUADS_PER_LARGE_GRASS * desc.largeGrassNumber;

	uint32_t largeGrassInstanceStartIndex = smallGrassInstanceNumber + mediumGrassInstanceNumber;

	instancesNumber = smallGrassInstanceNumber;
	instancesNumber += mediumGrassInstanceNumber;
	instancesNumber += largeGrassInstanceNumber;

	BufferDesc bufferDesc{};
	bufferDesc.numElements = instancesNumber;
	bufferDesc.dataStride = sizeof(Vegetation);
	bufferDesc.data.resize(static_cast<size_t>(bufferDesc.dataStride) * bufferDesc.numElements);

	if (std::filesystem::exists(desc.vegetationCacheFileName))
		LoadCache(desc.vegetationCacheFileName, bufferDesc.data.data(), bufferDesc.data.size());
	else
	{
		TextureDesc vegetationMapDesc{};
		DDSLoader::Load(desc.vegetationMapFileName, vegetationMapDesc);

		XMUBYTE4 mask{};
		mask.x = 255u;

		FillVegetationBuffer(bufferDesc.data.data(), vegetationMapDesc, desc, smallGrassInstanceNumber,
			0u, QUADS_PER_SMALL_GRASS, mask);

		mask.x = 0u;
		mask.y = 255u;

		FillVegetationBuffer(bufferDesc.data.data(), vegetationMapDesc, desc, mediumGrassInstanceNumber,
			smallGrassInstanceNumber, QUADS_PER_MEDIUM_GRASS, mask);

		mask.y = 0u;
		mask.z = 255u;

		FillVegetationBuffer(bufferDesc.data.data(), vegetationMapDesc, desc, largeGrassInstanceNumber,
			largeGrassInstanceStartIndex, QUADS_PER_LARGE_GRASS, mask);

		SaveCache(desc.vegetationCacheFileName, bufferDesc.data.data(), bufferDesc.data.size());
	}

	vegetationBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, bufferDesc);
}

void Common::Logic::SceneEntity::VegatationSystem::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	vegetationVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vegetationPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\VegetationPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VegatationSystem::LoadTextures(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const VegetationSystemDesc& desc)
{
	TextureDesc textureDesc{};
	DDSLoader::Load(desc.albedoMapFileName, textureDesc);
	albedoMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.normalMapFileName, textureDesc);
	normalMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::VegatationSystem::CreateMaterial(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, Graphics::Resources::ResourceID perlinNoiseId)
{
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto vegetationBufferResource = resourceManager->GetResource<Buffer>(vegetationBufferId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto albedoResource = resourceManager->GetResource<Texture>(albedoMapId);
	auto normalResource = resourceManager->GetResource<Texture>(normalMapId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto vegetationVS = resourceManager->GetResource<Shader>(vegetationVSId);
	auto vegetationPS = resourceManager->GetResource<Shader>(vegetationPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, vegetationBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, albedoResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(3u, normalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(_mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vegetationVS->bytecode);
	materialBuilder.SetPixelShader(vegetationPS->bytecode);

	_material = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::VegatationSystem::FillVegetationBuffer(uint8_t* buffer,
	const TextureDesc& vegetationMapDesc, const VegetationSystemDesc& desc, uint32_t elementNumber,
	uint32_t offset, uint32_t quadNumber, XMUBYTE4 mask)
{
	auto vegetations = reinterpret_cast<Vegetation*>(buffer);

	auto origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	auto identityQuaternion = XMQuaternionIdentity();
	auto scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	auto upVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	floatN position{};
	floatN upVectorTarget{};
	floatN rolling{};
	floatN rotation{};
	float2 atlasElementOffset{};

	for (uint32_t grassIndex = 0u; grassIndex < elementNumber; grassIndex++)
	{
		auto& vegetation = vegetations[grassIndex + offset];

		if (grassIndex % quadNumber == 0u)
		{
			GetRandomData(vegetationMapDesc, desc, mask, position, upVectorTarget, atlasElementOffset);
			rotation = CalculateRotation(upVector, upVectorTarget);
			rolling = XMQuaternionRotationNormal(upVectorTarget, static_cast<float>(std::numbers::pi * 2.0 / 3.0));
		}
		else
			rotation = XMQuaternionMultiply(rotation, rolling);

		vegetation.world = XMMatrixTransformation(origin, identityQuaternion, scale, origin, rotation, position);
		vegetation.atlasElementOffset = atlasElementOffset;
		vegetation.padding = {};
	}
}

void Common::Logic::SceneEntity::VegatationSystem::GetRandomData(const Graphics::Resources::TextureDesc& vegetationMapDesc,
	const VegetationSystemDesc& desc, XMUBYTE4 mask, floatN& position, floatN& upVector, float2& atlasOffset)
{
	auto mapSize = static_cast<uint32_t>(vegetationMapDesc.width * vegetationMapDesc.height);
	auto random = Utilities::Random(randomEngine);

	auto startIndex = std::min(static_cast<uint32_t>(random * mapSize), mapSize - 1u);

	auto vegetationDataPtr = reinterpret_cast<const XMUBYTE4*>(vegetationMapDesc.data.data());

	auto terrain = desc.terrain;
	auto& minCorner = terrain->GetMinCorner();
	auto& size = terrain->GetSize();

	for (uint32_t pixelIndex = 0; pixelIndex < mapSize; pixelIndex++)
	{
		auto fetchedIndex = (pixelIndex + startIndex) % mapSize;

		auto vegetationDataV = *(vegetationDataPtr + fetchedIndex);
		int32_t vegetationData = vegetationDataV.x & mask.x + vegetationDataV.y & mask.y + vegetationDataV.z & mask.z;

		if (vegetationData > 0)
		{
			atlasOffset.x = ((vegetationData - 1) % desc.atlasRows) / static_cast<float>(desc.atlasRows);
			atlasOffset.y = static_cast<float>((vegetationData - 1) / desc.atlasRows) / desc.atlasColumns;

			auto xCoeff = static_cast<float>(fetchedIndex % static_cast<uint32_t>(vegetationMapDesc.width));
			xCoeff /= vegetationMapDesc.width;

			auto yCoeff = static_cast<float>(fetchedIndex / static_cast<uint32_t>(vegetationMapDesc.width));
			yCoeff /= vegetationMapDesc.height;

			auto planarPosition = float2(minCorner.x + size.x * xCoeff, minCorner.y + size.y * yCoeff);
			auto height = terrain->GetHeight(planarPosition);
			auto normal = terrain->GetNormal(planarPosition);

			position = XMVectorSet(planarPosition.x, planarPosition.y, height, 0.0f);
			upVector = XMLoadFloat3(&normal);
		}
	}
}

floatN Common::Logic::SceneEntity::VegatationSystem::CalculateRotation(const floatN& upVector, const floatN& upVectorTarget)
{
	auto rotation = XMQuaternionRotationNormal(upVector,
		Utilities::Random(randomEngine) * static_cast<float>(std::numbers::pi * 2.0));
	
	auto angle = XMVector3Dot(upVectorTarget, upVector);
	
	if (std::abs(angle.m128_f32[0u] - 1.0f) < std::numeric_limits<float>::epsilon())
	{
		auto rotationAxis = XMVector3Cross(upVectorTarget, upVector);
		auto pitch = XMQuaternionRotationAxis(rotationAxis, angle.m128_f32[0u]);

		rotation = XMQuaternionMultiply(rotation, pitch);
	}

	return rotation;
}

void Common::Logic::SceneEntity::VegatationSystem::LoadCache(const std::filesystem::path& fileName,
	uint8_t* buffer, size_t size)
{
	std::ifstream vegetationFile(fileName, std::ios::binary);
	vegetationFile.read(reinterpret_cast<char*>(buffer), size);
}

void Common::Logic::SceneEntity::VegatationSystem::SaveCache(const std::filesystem::path& fileName,
	const uint8_t* buffer, size_t size)
{
	std::ofstream vegetationFile(fileName, std::ios::binary);
	vegetationFile.write(reinterpret_cast<const char*>(buffer), size);
}
