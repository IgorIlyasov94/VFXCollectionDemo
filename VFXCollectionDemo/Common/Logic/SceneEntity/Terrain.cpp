#include "Terrain.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/OBJLoader.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../../Graphics/Assets/GeometryUtilities.h"
#include "LightingSystem.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Graphics;
using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::Terrain::Terrain(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, const TerrainDesc& desc)
{
	verticesPerWidth = desc.verticesPerWidth;
	verticesPerHeight = desc.verticesPerHeight;

	depthPassConstants.world = DirectX::XMMatrixTranslation(desc.origin.x, desc.origin.y, desc.origin.z);

	minCorner = desc.origin;
	minCorner.x -= desc.size.x * 0.5f;
	minCorner.y -= desc.size.y * 0.5f;

	mapSize = desc.size;

	materialDepthPass = desc.materialDepthPass;
	materialDepthCubePass = desc.materialDepthCubePass;

	lightConstantBufferId = desc.lightConstantBufferId;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	auto loadCache = std::filesystem::exists(desc.terrainFileName);

	if (loadCache)
	{
		auto heightMapFileTimestamp = std::filesystem::last_write_time(desc.heightMapFileName);
		auto cacheFileTimestamp = std::filesystem::last_write_time(desc.terrainFileName);

		loadCache = cacheFileTimestamp >= heightMapFileTimestamp;
	}

	if (loadCache)
		LoadCache(desc.terrainFileName, device, commandList, resourceManager);
	else
	{
		LoadNormalHeightData(desc.heightMapFileName);
		GenerateMesh(desc.terrainFileName, desc.blendMapFileName, commandList, renderer);
	}

	CreateConstantBuffers(device, commandList, resourceManager, desc);
	LoadShaders(device, resourceManager, desc);
	LoadTextures(device, commandList, resourceManager, desc);
	CreateMaterial(device, resourceManager, desc);
}

Common::Logic::SceneEntity::Terrain::~Terrain()
{

}

const float4x4& Common::Logic::SceneEntity::Terrain::GetWorld() const
{
	return depthPassConstants.world;
}

float Common::Logic::SceneEntity::Terrain::GetHeight(const float2& position) const
{
	uint32_t index00;
	uint32_t index10;
	uint32_t index01;
	uint32_t index11;

	float4 barycentric;

	FetchCoord(position, index00, index10, index01, index11, barycentric);

	auto height = normalHeightData[index00].m128_f32[3u] * barycentric.x;
	height += normalHeightData[index10].m128_f32[3u] * barycentric.y;
	height += normalHeightData[index01].m128_f32[3u] * barycentric.z;
	height += normalHeightData[index11].m128_f32[3u] * barycentric.w;

	return height + minCorner.z;
}

float3 Common::Logic::SceneEntity::Terrain::GetNormal(const float2& position) const
{
	uint32_t index00;
	uint32_t index10;
	uint32_t index01;
	uint32_t index11;

	float4 barycentric;

	FetchCoord(position, index00, index10, index01, index11, barycentric);

	auto normalV = normalHeightData[index00] * barycentric.x;
	normalV += normalHeightData[index10] * barycentric.y;
	normalV += normalHeightData[index01] * barycentric.z;
	normalV += normalHeightData[index11] * barycentric.w;

	float3 normal;
	XMStoreFloat3(&normal, XMVector3Normalize(normalV));

	return normal;
}

const float3& Common::Logic::SceneEntity::Terrain::GetSize() const noexcept
{
	return mapSize;
}

const float3& Common::Logic::SceneEntity::Terrain::GetMinCorner() const noexcept
{
	return minCorner;
}

void Common::Logic::SceneEntity::Terrain::Update(const Camera* camera, float time)
{
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();
	mutableConstantsBuffer->cameraPosition = camera->GetPosition();
	mutableConstantsBuffer->time = time;
}

void Common::Logic::SceneEntity::Terrain::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{
	materialDepthPrepass->Set(commandList);
	mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::Terrain::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	depthPassConstants.lightMatrixStartIndex = lightMatrixStartIndex;

	materialDepthPass->Set(commandList);
	materialDepthPass->SetRootConstants(commandList, 0u, 17u, &depthPassConstants);
	mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::Terrain::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{
	depthPassConstants.lightMatrixStartIndex = lightMatrixStartIndex;

	materialDepthCubePass->Set(commandList);
	materialDepthCubePass->SetRootConstants(commandList, 0u, 17u, &depthPassConstants);
	mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::Terrain::Draw(ID3D12GraphicsCommandList* commandList)
{
	material->Set(commandList);
	mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::Terrain::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	mesh->Release(resourceManager);
	delete mesh;

	delete material;
	delete materialDepthPrepass;

	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Texture>(albedo0Id);
	resourceManager->DeleteResource<Texture>(albedo1Id);
	resourceManager->DeleteResource<Texture>(albedo2Id);
	resourceManager->DeleteResource<Texture>(albedo3Id);
	resourceManager->DeleteResource<Texture>(normal0Id);
	resourceManager->DeleteResource<Texture>(normal1Id);
	resourceManager->DeleteResource<Texture>(normal2Id);
	resourceManager->DeleteResource<Texture>(normal3Id);
	resourceManager->DeleteResource<Shader>(terrainVSId);
	resourceManager->DeleteResource<Shader>(terrainPSId);
}

void Common::Logic::SceneEntity::Terrain::LoadNormalHeightData(const std::filesystem::path& heightMapFileName)
{
	TextureDesc desc{};
	DDSLoader::Load(heightMapFileName, desc);

	auto pixelNumber = static_cast<uint32_t>(desc.width * desc.height);
	auto verticesNumber = static_cast<uint32_t>(static_cast<uint64_t>(verticesPerWidth) * verticesPerHeight);

	normalHeightData.resize(verticesNumber);
	
	auto lastPtr = reinterpret_cast<const uint8_t*>(desc.data.data() + pixelNumber - 1);
	auto heightPtr = reinterpret_cast<const uint8_t*>(desc.data.data());
	auto cellSize = float2(mapSize.x / verticesPerWidth, mapSize.y / verticesPerHeight);

	auto incrementX = static_cast<uint32_t>(std::ceil(static_cast<float>(desc.width) / verticesPerWidth));
	auto incrementY = static_cast<uint32_t>(std::ceil(static_cast<float>(desc.height) / verticesPerHeight));
	incrementY *= static_cast<uint32_t>(desc.width);
	
	for (uint32_t indexY = 0u; indexY < verticesPerHeight; indexY++)
	{
		for (uint32_t indexX = 0u; indexX < verticesPerWidth; indexX++)
		{
			auto offset = static_cast<uint32_t>(static_cast<float>(indexX * desc.width) / verticesPerWidth);
			offset += static_cast<uint32_t>(desc.width * static_cast<uint64_t>(static_cast<float>(indexY * desc.height) / verticesPerHeight));
			auto height = *std::min(heightPtr + offset, lastPtr) * mapSize.z / 255.0f;

			auto heightX = *(std::min(heightPtr + offset + incrementX, lastPtr)) * mapSize.z / 255.0f;
			auto heightY = *(std::min(heightPtr + offset + ((heightPtr + incrementY) <= lastPtr ? incrementY : 0u), lastPtr)) * mapSize.z / 255.0f;

			auto position = XMVectorSet
			(
				cellSize.x * indexX + minCorner.x,
				cellSize.y * indexY + minCorner.y,
				height + minCorner.y,
				0.0f
			);

			auto positionX = XMVectorSet
			(
				position.m128_f32[0u] + cellSize.x,
				position.m128_f32[1u],
				heightX + minCorner.y,
				0.0f
			);

			auto positionY = XMVectorSet
			(
				position.m128_f32[0u],
				position.m128_f32[1u] + cellSize.y,
				heightY + minCorner.y,
				0.0f
			);

			auto Vx = positionX - position;
			auto Vy = positionY - position;

			auto normal = XMVector3Cross(Vx, Vy);
			normal = XMVector3Normalize(normal);

			auto dataIndex = indexX + indexY * verticesPerWidth;

			normalHeightData[dataIndex] = XMVectorSet(normal.m128_f32[0u], normal.m128_f32[1u], normal.m128_f32[2u], height);
		}
	}
}

void Common::Logic::SceneEntity::Terrain::GenerateMesh(const std::filesystem::path& terrainFileName,
	const std::filesystem::path& blendMapFileName, ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	auto verticesNumber = static_cast<uint32_t>(verticesPerWidth * verticesPerHeight);
	
	BufferDesc vbDesc{};
	vbDesc.dataStride = sizeof(TerrainVertex);
	vbDesc.numElements = verticesNumber;
	vbDesc.data.resize(static_cast<size_t>(vbDesc.numElements) * vbDesc.dataStride, 0u);
	
	auto cellSize = float2(mapSize.x / (verticesPerWidth - 1), mapSize.y / (verticesPerHeight - 1));

	auto vertices = reinterpret_cast<TerrainVertex*>(vbDesc.data.data());
	
	for (uint32_t vertexIndexY = 0u; vertexIndexY < verticesPerHeight; vertexIndexY++)
	{
		for (uint32_t vertexIndexX = 0u; vertexIndexX < verticesPerWidth; vertexIndexX++)
		{
			uint32_t vertexIndex = vertexIndexX + vertexIndexY * verticesPerWidth;

			auto& vertex = vertices[vertexIndex];

			auto positionXY = float2(vertexIndexX * cellSize.x, vertexIndexY * cellSize.y);
			positionXY.x += minCorner.x;
			positionXY.y += minCorner.y;

			vertex.position.x = positionXY.x;
			vertex.position.y = positionXY.y;
			vertex.position.z = GetHeight(positionXY);

			auto normal = GetNormal(positionXY);

			vertex.normalX = XMConvertFloatToHalf(normal.x);
			vertex.normalY = XMConvertFloatToHalf(normal.y);
			vertex.normalZ = XMConvertFloatToHalf(normal.z);
			vertex.normalW = XMConvertFloatToHalf(0.0f);

			auto tangent = GeometryUtilities::CalculateTangent(normal);

			vertex.tangentX = XMConvertFloatToHalf(tangent.x);
			vertex.tangentY = XMConvertFloatToHalf(tangent.y);
			vertex.tangentZ = XMConvertFloatToHalf(tangent.z);
			vertex.tangentW = XMConvertFloatToHalf(0.0f);

			float2 texCoord
			{
				static_cast<float>(vertexIndexX) / (verticesPerWidth - 1),
				static_cast<float>(vertexIndexY) / (verticesPerHeight - 1)
			};

			vertex.texCoordX = XMConvertFloatToHalf(texCoord.x);
			vertex.texCoordY = XMConvertFloatToHalf(texCoord.y);
		}
	}

	auto cellsNumber = static_cast<uint32_t>((verticesPerWidth - 1) * (verticesPerHeight - 1));
	auto indicesNumber = cellsNumber * 6u;

	BufferDesc ibDesc{};
	ibDesc.dataStride = indicesNumber < std::numeric_limits<uint16_t>::max() ? 2u : 4u;
	ibDesc.numElements = indicesNumber;
	ibDesc.data.resize(static_cast<size_t>(ibDesc.numElements) * ibDesc.dataStride);

	if (ibDesc.dataStride == 2u)
		FillIndices(reinterpret_cast<uint16_t*>(ibDesc.data.data()), indicesNumber);
	else
		FillIndices(reinterpret_cast<uint32_t*>(ibDesc.data.data()), indicesNumber);
	
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vbDesc);

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, ibDesc);

	MeshDesc meshDesc{};
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::NORMAL | VertexFormat::TANGENT |
		VertexFormat::TEXCOORD0 | VertexFormat::BLENDWEIGHT;

	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	meshDesc.indexFormat = ibDesc.dataStride == 2u ? IndexFormat::UINT16_INDEX : IndexFormat::UINT32_INDEX;
	meshDesc.verticesNumber = verticesNumber;
	meshDesc.indicesNumber = indicesNumber;

	mesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);

	SaveCache(terrainFileName, meshDesc, vbDesc.data, ibDesc.data);
}

void Common::Logic::SceneEntity::Terrain::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const TerrainDesc& desc)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->mapTiling0 = desc.map0Tiling;
	mutableConstantsBuffer->mapTiling1 = desc.map1Tiling;
	mutableConstantsBuffer->mapTiling2 = desc.map2Tiling;
	mutableConstantsBuffer->mapTiling3 = desc.map3Tiling;
	mutableConstantsBuffer->zNear = LightingSystem::SHADOW_MAP_Z_NEAR;
	mutableConstantsBuffer->zFar = LightingSystem::SHADOW_MAP_Z_FAR;
	mutableConstantsBuffer->padding = {};
}

void Common::Logic::SceneEntity::Terrain::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, const TerrainDesc& desc)
{
	terrainVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\TerrainVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	terrainPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\TerrainPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5, *desc.lightDefines);
}

void Common::Logic::SceneEntity::Terrain::LoadTextures(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager,
	const TerrainDesc& desc)
{
	TextureDesc textureDesc{};
	DDSLoader::Load(desc.map0AlbedoFileName, textureDesc);
	albedo0Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map1AlbedoFileName, textureDesc);
	albedo1Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map2AlbedoFileName, textureDesc);
	albedo2Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map3AlbedoFileName, textureDesc);
	albedo3Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load(desc.map0NormalFileName, textureDesc);
	normal0Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map1NormalFileName, textureDesc);
	normal1Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map2NormalFileName, textureDesc);
	normal2Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load(desc.map3NormalFileName, textureDesc);
	normal3Id = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load(desc.blendMapFileName, textureDesc);
	blendMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::Terrain::CreateMaterial(ID3D12Device* device, ResourceManager* resourceManager,
	const TerrainDesc& desc)
{
	auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightConstantBufferId);
	auto constantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto blendMapResource = resourceManager->GetResource<Texture>(blendMapId);
	auto albedo0Resource = resourceManager->GetResource<Texture>(albedo0Id);
	auto albedo1Resource = resourceManager->GetResource<Texture>(albedo1Id);
	auto albedo2Resource = resourceManager->GetResource<Texture>(albedo2Id);
	auto albedo3Resource = resourceManager->GetResource<Texture>(albedo3Id);
	auto normal0Resource = resourceManager->GetResource<Texture>(normal0Id);
	auto normal1Resource = resourceManager->GetResource<Texture>(normal1Id);
	auto normal2Resource = resourceManager->GetResource<Texture>(normal2Id);
	auto normal3Resource = resourceManager->GetResource<Texture>(normal3Id);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_ANISOTROPIC_WRAP);
	auto samplerShadowResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_COMPARISON_POINT_CLAMP,
		Graphics::DefaultFilterComparisonFunc::COMPARISON_LESS_EQUAL);

	auto terrainVS = resourceManager->GetResource<Shader>(terrainVSId);
	auto terrainPS = resourceManager->GetResource<Shader>(terrainPSId);
	
	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_OPAQUE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, lightConstantBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetConstantBuffer(1u, constantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, albedo0Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, albedo1Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(2u, albedo2Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(3u, albedo3Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(4u, normal0Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(5u, normal1Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(6u, normal2Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(7u, normal3Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(8u, blendMapResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);

	for (uint32_t shadowMapIndex = 0u; shadowMapIndex < desc.shadowMapIds.size(); shadowMapIndex++)
	{
		auto shadowMapResource = resourceManager->GetResource<Texture>(desc.shadowMapIds[shadowMapIndex]);
		materialBuilder.SetTexture(9u + shadowMapIndex, shadowMapResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	if (desc.hasParticleLighting)
	{
		auto particleLightBufferResource = resourceManager->GetResource<RWBuffer>(desc.lightParticleBufferId);
		materialBuilder.SetBuffer(10u, particleLightBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(1u, samplerShadowResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);

	materialBuilder.SetCullMode(D3D12_CULL_MODE_FRONT);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, true, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(terrainVS->bytecode);
	materialBuilder.SetPixelShader(terrainPS->bytecode);

	material = materialBuilder.ComposeStandard(device);

	materialBuilder.SetConstantBuffer(1u, constantsResource->resourceGPUAddress);
	
	materialBuilder.SetCullMode(D3D12_CULL_MODE_FRONT);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, true, false, true);
	materialBuilder.SetGeometryFormat(mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(terrainVS->bytecode);
	
	materialDepthPrepass = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::Terrain::LoadCache(const std::filesystem::path& fileName, ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	MeshDesc meshDesc{};

	std::ifstream terrainFile(fileName, std::ios::binary);
	terrainFile.read(reinterpret_cast<char*>(&meshDesc), sizeof(MeshDesc));

	BufferDesc vbDesc{};
	vbDesc.dataStride = sizeof(TerrainVertex);
	vbDesc.numElements = meshDesc.verticesNumber;

	auto vertexBufferSize = static_cast<size_t>(meshDesc.verticesNumber) * vbDesc.dataStride;
	vbDesc.data.resize(vertexBufferSize);
	terrainFile.read(reinterpret_cast<char*>(vbDesc.data.data()), vertexBufferSize);

	BufferDesc ibDesc{};
	ibDesc.dataStride = meshDesc.indexFormat == IndexFormat::UINT16_INDEX ? 2u : 4u;
	ibDesc.numElements = meshDesc.indicesNumber;

	auto indexBufferSize = static_cast<size_t>(meshDesc.indicesNumber) * ibDesc.dataStride;
	ibDesc.data.resize(indexBufferSize);
	terrainFile.read(reinterpret_cast<char*>(ibDesc.data.data()), indexBufferSize);

	uint32_t normalHeightDataSize{};
	terrainFile.read(reinterpret_cast<char*>(&normalHeightDataSize), sizeof(uint32_t));

	normalHeightData.resize(normalHeightDataSize);

	terrainFile.read(reinterpret_cast<char*>(normalHeightData.data()), sizeof(float4) * normalHeightDataSize);

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vbDesc);

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, ibDesc);

	mesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::Terrain::SaveCache(const std::filesystem::path& fileName,
	const MeshDesc& meshDesc, const std::vector<uint8_t>& verticesData, const std::vector<uint8_t>& indicesData) const
{
	std::ofstream meshFile(fileName, std::ios::binary);
	meshFile.write(reinterpret_cast<const char*>(&meshDesc), sizeof(MeshDesc));
	meshFile.write(reinterpret_cast<const char*>(verticesData.data()), verticesData.size());
	meshFile.write(reinterpret_cast<const char*>(indicesData.data()), indicesData.size());

	auto normalHeightDataSize = static_cast<uint32_t>(normalHeightData.size());

	meshFile.write(reinterpret_cast<const char*>(&normalHeightDataSize), sizeof(uint32_t));
	meshFile.write(reinterpret_cast<const char*>(normalHeightData.data()), normalHeightDataSize * sizeof(float4));
}

void Common::Logic::SceneEntity::Terrain::FetchCoord(const float2& position, uint32_t& index00, uint32_t& index10,
	uint32_t& index01, uint32_t& index11, float4& barycentricCoord) const
{
	float2 fetchedPosition
	{
		(position.x - minCorner.x) / mapSize.x,
		(position.y - minCorner.y) / mapSize.y
	};

	auto positionAverage = float3(fetchedPosition.x * verticesPerWidth, fetchedPosition.y * verticesPerHeight, 0.0f);
	
	auto positionMin = float2(std::floor(positionAverage.x), std::floor(positionAverage.y));
	auto positionMax = float2(positionMin.x + 1.0f, positionMin.y + 1.0f);

	index00 = std::min(static_cast<uint32_t>(std::lroundf(std::min(positionMin.x, verticesPerWidth - 1.0f) +
		positionMin.y * verticesPerWidth)), static_cast<uint32_t>(normalHeightData.size()) - 1u);

	index10 = std::min(static_cast<uint32_t>(std::lroundf(std::min(positionMax.x, verticesPerWidth - 1.0f) +
		positionMax.y * verticesPerWidth)), static_cast<uint32_t>(normalHeightData.size()) - 1u);

	index01 = std::min(index00 + verticesPerWidth, static_cast<uint32_t>(normalHeightData.size()) - 1u);
	index11 = std::min(index10 + verticesPerWidth, static_cast<uint32_t>(normalHeightData.size()) - 1u);

	auto cellSize = float2(positionMax.x - positionMin.x, positionMax.y - positionMin.y);
	
	auto position00 = float3(positionMin.x, positionMin.y, 0.0f);
	auto position01 = float3(positionMin.x, positionMax.y, 0.0f);
	auto position11 = float3(positionMax.x, positionMax.y, 0.0f);
	auto position10 = float3(positionMax.x, positionMin.y, 0.0f);
	
	auto linearY = (positionAverage.x - positionMin.x) * cellSize.y / cellSize.x + positionMin.y;

	if (positionAverage.y > linearY)
	{
		auto barycentrics = GeometryUtilities::CalculateBarycentric(position00, position01, position11, positionAverage);

		barycentricCoord.x = barycentrics.x;
		barycentricCoord.y = barycentrics.y;
		barycentricCoord.z = barycentrics.z;
		barycentricCoord.w = 0.0f;
	}
	else
	{
		auto barycentrics = GeometryUtilities::CalculateBarycentric(position11, position10, position00, positionAverage);

		barycentricCoord.x = barycentrics.z;
		barycentricCoord.y = 0.0f;
		barycentricCoord.z = barycentrics.x;
		barycentricCoord.w = barycentrics.y;
	}
}
