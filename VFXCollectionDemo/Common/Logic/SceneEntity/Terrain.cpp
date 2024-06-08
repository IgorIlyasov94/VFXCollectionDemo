#include "Terrain.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/OBJLoader.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../../Graphics/Assets/GeometryUtilities.h"

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

	minCorner = desc.origin;
	minCorner.x -= desc.size.x * 0.5f;
	minCorner.y -= desc.size.y * 0.5f;
	minCorner.z -= desc.size.z * 0.5f;

	mapSize = desc.size;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	if (std::filesystem::exists(desc.terrainFileName))
		LoadCache(desc.terrainFileName, device, commandList, resourceManager);
	else
	{
		LoadNormalHeightData(desc.heightMapFileName);
		GenerateMesh(desc.terrainFileName, desc.blendMapFileName, commandList, renderer);
	}

	CreateConstantBuffers(device, commandList, resourceManager, desc);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager, desc);
	CreateMaterial(device, resourceManager);
}

Common::Logic::SceneEntity::Terrain::~Terrain()
{

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

	return height;
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

void Common::Logic::SceneEntity::Terrain::Draw(ID3D12GraphicsCommandList* commandList, const Camera* camera,
	float time)
{
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();
	mutableConstantsBuffer->time = time;

	material->Set(commandList);
	mesh->Draw(commandList);
}

void Common::Logic::SceneEntity::Terrain::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	mesh->Release(resourceManager);
	delete mesh;

	delete material;

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

	normalHeightData.resize(pixelNumber);

	auto lastPtr = reinterpret_cast<const float*>(desc.data.data() + pixelNumber - 1);
	auto heightPtr = reinterpret_cast<const float*>(desc.data.data());
	auto cellSize = float2(mapSize.x / desc.width, mapSize.y / desc.height);

	for (uint32_t pixelIndex = 0u; pixelIndex < pixelNumber; pixelIndex++)
	{
		auto height = *heightPtr;
		height *= mapSize.z;

		auto heightX = *(std::min(heightPtr + 1, lastPtr));
		auto heightY = *(heightPtr + ((heightPtr + desc.width) <= lastPtr ? desc.width : 0u));

		float dx = (heightX - height) / cellSize.x;
		float dy = (heightY - height) / cellSize.y;

		auto normal = XMVectorSet(-dx, -dy, 1.0f, 0.0f);
		normal = XMVector3Normalize(normal);

		normalHeightData[pixelIndex] = XMVectorSet(normal.m128_f32[0u], normal.m128_f32[1u], normal.m128_f32[2u], height);

		heightPtr++;
	}

	mapWidth = static_cast<uint32_t>(desc.width);
	mapHeight = desc.height;
}

void Common::Logic::SceneEntity::Terrain::GenerateMesh(const std::filesystem::path& terrainFileName,
	const std::filesystem::path& blendMapFileName, ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	auto verticesNumber = static_cast<uint32_t>(verticesPerWidth * verticesPerHeight);
	
	BufferDesc vbDesc{};
	vbDesc.dataStride = sizeof(TerrainVertex);
	vbDesc.numElements = verticesNumber;
	vbDesc.data.resize(static_cast<size_t>(vbDesc.numElements) * vbDesc.dataStride);
	
	auto cellSize = float2(mapSize.x / (verticesPerWidth - 1), mapSize.y / (verticesPerHeight - 1));

	auto vertices = reinterpret_cast<TerrainVertex*>(vbDesc.data.data());
	
	TextureDesc blendMapDesc{};
	DDSLoader::Load(blendMapFileName, blendMapDesc);

	auto blendDataPtr = reinterpret_cast<const uint32_t*>(blendMapDesc.data.data());
	
	for (uint32_t vertexIndexY = 0u; vertexIndexY < verticesPerHeight; vertexIndexY++)
	{
		for (uint32_t vertexIndexX = 0u; vertexIndexX < verticesPerWidth; vertexIndexX++)
		{
			uint32_t vertexIndex = vertexIndexX + vertexIndexY * verticesPerWidth;

			auto& vertex = vertices[vertexIndex];

			auto positionXY = float2(vertexIndexX * cellSize.x, vertexIndexY * cellSize.y);

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

			size_t blendDataOffset = static_cast<size_t>(std::lroundf(texCoord.x * (blendMapDesc.width - 1)));
			blendDataOffset += static_cast<size_t>(std::lroundf(texCoord.y * (blendMapDesc.height - 1))) * blendMapDesc.width;

			vertex.blendWeight = *(blendDataPtr + blendDataOffset);

			uint32_t R = (vertex.blendWeight & 0xFF000000) >> 16;
			uint32_t B = (vertex.blendWeight & 0x0000FF00) << 16;

			vertex.blendWeight = (vertex.blendWeight & 0x00FF00FF) | R | B;
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
}

void Common::Logic::SceneEntity::Terrain::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	terrainVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\TerrainVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	terrainPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\TerrainPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
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
}

void Common::Logic::SceneEntity::Terrain::CreateMaterial(ID3D12Device* device, ResourceManager* resourceManager)
{
	auto constantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto albedo0Resource = resourceManager->GetResource<Texture>(albedo0Id);
	auto albedo1Resource = resourceManager->GetResource<Texture>(albedo1Id);
	auto albedo2Resource = resourceManager->GetResource<Texture>(albedo2Id);
	auto albedo3Resource = resourceManager->GetResource<Texture>(albedo3Id);
	auto normal0Resource = resourceManager->GetResource<Texture>(normal0Id);
	auto normal1Resource = resourceManager->GetResource<Texture>(normal1Id);
	auto normal2Resource = resourceManager->GetResource<Texture>(normal2Id);
	auto normal3Resource = resourceManager->GetResource<Texture>(normal3Id);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto terrainVS = resourceManager->GetResource<Shader>(terrainVSId);
	auto terrainPS = resourceManager->GetResource<Shader>(terrainPSId);

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_OPAQUE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, constantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, albedo0Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, albedo1Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(2u, albedo2Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(3u, albedo3Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(4u, normal0Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(5u, normal1Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(6u, normal2Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(7u, normal3Resource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_BACK);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(mesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(terrainVS->bytecode);
	materialBuilder.SetPixelShader(terrainPS->bytecode);

	material = materialBuilder.ComposeStandard(device);
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

	auto positionMin = float2(std::floor(fetchedPosition.x * mapWidth), std::floor(fetchedPosition.y * mapHeight));
	auto positionMax = float2(std::ceil(fetchedPosition.x * mapWidth), std::ceil(fetchedPosition.y * mapHeight));

	index00 = static_cast<uint32_t>(std::lroundf(positionMin.x + positionMin.y * mapWidth));
	index10 = static_cast<uint32_t>(std::lroundf(positionMax.x + positionMax.y * mapWidth));

	index01 = std::min(index00 + mapWidth, static_cast<uint32_t>(normalHeightData.size()) - 1u);
	index11 = std::min(index10 + mapWidth, static_cast<uint32_t>(normalHeightData.size()) - 1u);

	auto cellSize = float2(positionMax.x - positionMin.x, positionMax.y - positionMin.y);
	auto area = std::max(cellSize.x * cellSize.y, 0.0001f);

	auto positionAverage = XMVectorSet(fetchedPosition.x * mapWidth, fetchedPosition.y * mapHeight, 0.0f, 0.0f);

	auto position00 = XMLoadFloat2(&positionMin);
	auto position10 = XMVectorSet(positionMax.x, positionMin.y, 0.0f, 0.0f);
	auto position01 = XMVectorSet(positionMin.x, positionMax.y, 0.0f, 0.0f);
	auto position11 = XMLoadFloat2(&positionMax);

	barycentricCoord.x = XMVector2Length(positionAverage - position00).m128_f32[0u] / area;
	barycentricCoord.y = XMVector2Length(positionAverage - position10).m128_f32[0u] / area;
	barycentricCoord.z = XMVector2Length(positionAverage - position01).m128_f32[0u] / area;
	barycentricCoord.w = XMVector2Length(positionAverage - position11).m128_f32[0u] / area;
}
