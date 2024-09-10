#include "Scene_1_Whiteroom.h"
#include "../../Graphics/Assets/Loaders/OBJLoader.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/RaytracingObjectBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace DirectX;
using namespace Graphics;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;
using namespace Common::Logic::SceneEntity;

Common::Logic::Scene::Scene_1_WhiteRoom::Scene_1_WhiteRoom()
	: isLoaded(false), lightingObject(nullptr), camera(nullptr), lightingSystem(nullptr),
	mutableConstantsBuffer{}, mutableConstantsId{}, resultTargetResource{},
	whiteroomAlbedoId{}, whiteroomNormalId{}, resultTargetId{}, timer{}, _deltaTime{}, fps(60.0f),
	cpuTimeCounter{}, prevTimePoint{}, frameCounter{}, whiteroomVertexBufferId{},
	whiteroomIndexBufferId{}, whiteroomVertexBufferResource{}, whiteroomIndexBufferResource{}, lightingRSId{},
	postProcessManager{}, groupsNumberX(1u), groupsNumberY(1u), accelerationStructureDesc{}
{
	cameraPosition = float3(1.0f, 1.0f, 3.0f);

	cameraLookAt = {};

	cameraUpVector = float3(0.0f, 0.0f, 1.0f);
	auto cameraUpVectorN = DirectX::XMLoadFloat3(&cameraUpVector);
	auto cameraPositionN = DirectX::XMLoadFloat3(&cameraPosition);
	cameraUpVectorN += cameraPositionN;

	XMStoreFloat3(&cameraUpVector, cameraUpVectorN);

	barriers.reserve(MAX_SIMULTANEOUS_BARRIER_NUMBER);
}

Common::Logic::Scene::Scene_1_WhiteRoom::~Scene_1_WhiteRoom()
{

}

void Common::Logic::Scene::Scene_1_WhiteRoom::Load(Graphics::DirectX12Renderer* renderer)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	auto commandList = renderer->StartCreatingResources();

	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	groupsNumberX = width == 0u ? 1u : (1u + (width - 1) / THREADS_PER_GROUP_X);
	groupsNumberY = height == 0u ? 1u : (1u + (height - 1) / THREADS_PER_GROUP_Y);

	CreateLights(renderer);
	LoadMeshes(device, commandList, resourceManager);
	CreateConstantBuffers(device, resourceManager, width, height);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);
	CreateTargets(device, commandList, resourceManager, width, height);
	CreateObjects(device, commandList, renderer, resourceManager);

	quadMesh = postProcessManager->GetFullscreenQuadMesh();

	renderer->EndCreatingResources();

	prevTimePoint = std::chrono::high_resolution_clock::now();

	isLoaded = true;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::Unload(Graphics::DirectX12Renderer* renderer)
{
	auto resourceManager = renderer->GetResourceManager();
	auto bufferManager = renderer->GetBufferManager();

	lightingObject->Release(bufferManager);
	delete lightingObject;

	delete texturedQuadMaterial;

	delete lightingSystem;

	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Buffer>(whiteroomVertexBufferId);
	resourceManager->DeleteResource<Buffer>(whiteroomIndexBufferId);
	resourceManager->DeleteResource<Texture>(whiteroomAlbedoId);
	resourceManager->DeleteResource<Texture>(whiteroomNormalId);

	resourceManager->DeleteResource<RWTexture>(resultTargetId);

	resourceManager->DeleteResource<Shader>(lightingRSId);
	resourceManager->DeleteResource<Shader>(quadVSId);
	resourceManager->DeleteResource<Shader>(quadPSId);

	delete camera;
	
	postProcessManager->Release(resourceManager);
	delete postProcessManager;

	accelerationStructureDesc = {};

	isLoaded = false;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::OnResize(Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);

	mutableConstantsBuffer->invViewProjection = camera->GetInvViewProjection();

	auto resourceManager = renderer->GetResourceManager();
	resourceManager->DeleteResource<RWTexture>(resultTargetId);

	auto commandList = renderer->StartCreatingResources();
	CreateTargets(renderer->GetDevice(), commandList, resourceManager, width, height);
	renderer->EndCreatingResources();

	postProcessManager->OnResize(renderer);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::Update()
{
	cameraPosition.x = std::cos(timer * 0.2f) * 5.0f;
	cameraPosition.y = std::sin(timer * 0.2f) * 5.0f;
	cameraPosition.z = 3.0f;

	camera->Update(cameraPosition, cameraLookAt, cameraUpVector);

	mutableConstantsBuffer->invViewProjection = camera->GetInvViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();

	auto currentTimePoint = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimePoint - prevTimePoint);

	_deltaTime = 1.0f / fps;
	timer += _deltaTime;

	cpuTimeCounter += deltaTime.count();
	frameCounter++;

	if (cpuTimeCounter >= 1000u)
	{
		fps = static_cast<float>(std::min(frameCounter, 300u));
		cpuTimeCounter = 0u;
		frameCounter = 0u;
	}

	prevTimePoint = currentTimePoint;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::RenderShadows(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::Scene::Scene_1_WhiteRoom::Render(ID3D12GraphicsCommandList* commandList)
{
	ID3D12GraphicsCommandList5* commandList5 = nullptr;

	commandList->QueryInterface(IID_PPV_ARGS(&commandList5));

	resultTargetResource->EndBarrier(commandList);

	lightingObject->Dispatch(commandList5, groupsNumberX, groupsNumberY, 1u);

	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	resultTargetResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);

	if (resultTargetResource->GetBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(barriers.size(), barriers.data());

	postProcessManager->SetGBuffer(commandList);

	texturedQuadMaterial->Set(commandList);
	quadMesh->Draw(commandList);

	resultTargetResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	postProcessManager->Render(commandList);

	commandList5->Release();
}

void Common::Logic::Scene::Scene_1_WhiteRoom::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	postProcessManager->RenderToBackBuffer(commandList);
}

bool Common::Logic::Scene::Scene_1_WhiteRoom::IsLoaded()
{
	return isLoaded;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc vbDesc{};
	BufferDesc ibDesc{};

	std::filesystem::path filePath("Resources\\Meshes\\Whiteroom.obj");
	std::filesystem::path filePathCache("Resources\\Meshes\\Whiteroom.objCACHE");

	auto loadCache = std::filesystem::exists(filePathCache);

	if (loadCache)
	{
		auto mainFileTimestamp = std::filesystem::last_write_time(filePath);
		auto cacheFileTimestamp = std::filesystem::last_write_time(filePathCache);

		loadCache = cacheFileTimestamp >= mainFileTimestamp;
	}

	MeshDesc meshDesc{};

	if (loadCache)
	{
		LoadCache(filePathCache, meshDesc, vbDesc.data, ibDesc.data);
	}
	else
	{
		OBJLoader::Load(filePath, false, true, meshDesc, vbDesc.data, ibDesc.data);

		SaveCache(filePathCache, meshDesc, vbDesc.data, ibDesc.data);
	}

	vbDesc.dataStride = static_cast<uint32_t>(vbDesc.data.size() / meshDesc.verticesNumber);
	ibDesc.dataStride = 1u;
	ibDesc.numElements = ibDesc.data.size();

	whiteroomVertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, vbDesc);
	whiteroomIndexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, ibDesc);

	auto vertexBuffer = resourceManager->GetResource<VertexBuffer>(whiteroomVertexBufferId);
	auto indexBuffer = resourceManager->GetResource<IndexBuffer>(whiteroomIndexBufferId);

	whiteroomVertexBufferResource = vertexBuffer->resource;
	whiteroomIndexBufferResource = indexBuffer->resource;

	whiteroomVertexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
	whiteroomIndexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);

	accelerationStructureDesc.vertexBufferAddress = vertexBuffer->viewDesc.BufferLocation;
	accelerationStructureDesc.indexBufferAddress = indexBuffer->viewDesc.BufferLocation;
	accelerationStructureDesc.vertexStride = vertexBuffer->viewDesc.StrideInBytes;
	accelerationStructureDesc.indexStride = 2u;
	accelerationStructureDesc.verticesNumber = meshDesc.verticesNumber;
	accelerationStructureDesc.indicesNumber = meshDesc.indicesNumber;
	accelerationStructureDesc.isOpaque = true;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateConstantBuffers(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera = new Camera(cameraPosition, cameraLookAt, cameraUpVector, FOV_Y, aspectRatio, Z_NEAR, Z_FAR);

	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, nullptr, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->invViewProjection = camera->GetInvViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	lightingRSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\Lighting\\LightingRS.hlsl",
		ShaderType::RAYTRACING_SHADER, ShaderVersion::SM_6_5);

	quadVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PostProcesses\\QuadVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);
	quadPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PostProcesses\\TexturedPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\Whiteroom_AlbedoRoughness.dds", textureDesc);
	whiteroomAlbedoId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\Whiteroom_NormalMetalness.dds", textureDesc);
	whiteroomNormalId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateLights(Graphics::DirectX12Renderer* renderer)
{
	lightingSystem = new LightingSystem(renderer);

	LightDesc pointLight0{};
	pointLight0.position = float3(4.5f, -4.5f, 0.55f);
	pointLight0.color = float3(1.0f, 0.95f, 0.93f);
	pointLight0.intensity = lightIntensity0;
	pointLight0.type = LightType::POINT_LIGHT;

	pointLight0Id = lightingSystem->CreateLight(pointLight0);

	LightDesc pointLight1{};
	pointLight1.position = float3(-4.5f, 4.5f, 4.5f);
	pointLight1.color = float3(0.3f, 0.4f, 0.5f);
	pointLight1.intensity = lightIntensity1;
	pointLight1.type = LightType::AMBIENT_LIGHT;

	pointLight1Id = lightingSystem->CreateLight(pointLight1);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	TextureDesc targetDesc{};
	targetDesc.format = DirectX12Renderer::BACK_BUFFER_FORMAT;
	targetDesc.width = width;
	targetDesc.height = height;
	targetDesc.depth = 1u;
	targetDesc.mipLevels = 1u;
	targetDesc.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	targetDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	resultTargetId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::RW_TEXTURE, targetDesc);

	auto resultTarget = resourceManager->GetResource<RWTexture>(resultTargetId);
	resultTargetResource = resultTarget->resource;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateMaterials(ID3D12Device* device, Graphics::DirectX12Renderer* renderer,
	Graphics::Resources::ResourceManager* resourceManager)
{
	auto resultTargetTextureResource = resourceManager->GetResource<Texture>(resultTargetId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_POINT_CLAMP);

	auto quadVS = resourceManager->GetResource<Shader>(quadVSId);
	auto quadPS = resourceManager->GetResource<Shader>(quadPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetTexture(0u, resultTargetTextureResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, renderer->BACK_BUFFER_FORMAT);
	materialBuilder.SetGeometryFormat(quadMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(quadVS->bytecode);
	materialBuilder.SetPixelShader(quadPS->bytecode);

	texturedQuadMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, Graphics::Resources::ResourceManager* resourceManager)
{
	auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightingSystem->GetLightConstantBufferId());
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto whiteroomAlbedo = resourceManager->GetResource<Texture>(whiteroomAlbedoId);
	auto whiteroomNormal = resourceManager->GetResource<Texture>(whiteroomNormalId);
	auto linearSampler = resourceManager->GetDefaultSampler(device, DefaultFilterSetup::FILTER_BILINEAR_CLAMP);

	auto lightingRS = resourceManager->GetResource<Shader>(lightingRSId);

	RaytracingHitGroup hitGroup{};
	hitGroup.hitGroupName = L"TriangleHitGroup";
	hitGroup.closestHitShaderName = L"ClosestHit";
	
	RaytracingHitGroup shadowHitGroup{};
	shadowHitGroup.hitGroupName = L"ShadowHitGroup";

	RaytracingLibraryDesc libraryDesc{};
	libraryDesc.maxRecursionLevel = 2u;
	libraryDesc.dxilLibrary = lightingRS->bytecode;
	libraryDesc.triangleHitGroups.push_back(hitGroup);
	libraryDesc.triangleHitGroups.push_back(shadowHitGroup);
	libraryDesc.rayGenerationShaderName = L"RayGeneration";
	libraryDesc.missShaderNames.push_back(L"Miss");
	libraryDesc.missShaderNames.push_back(L"Miss_Shadow");
	libraryDesc.attributeStride = accelerationStructureDesc.vertexStride;
	libraryDesc.payloadStride = 16u;

	ID3D12Device5* device5 = nullptr;
	device->QueryInterface(IID_PPV_ARGS(&device5));

	ID3D12GraphicsCommandList5* commandList5 = nullptr;
	commandList->QueryInterface(IID_PPV_ARGS(&commandList5));

	whiteroomAlbedo->resource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	whiteroomNormal->resource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	RaytracingObjectBuilder raytracingBuilder{};
	raytracingBuilder.AddAccelerationStructure(accelerationStructureDesc);
	raytracingBuilder.SetConstantBuffer(0, lightConstantBufferResource->resourceGPUAddress);
	raytracingBuilder.SetConstantBuffer(1, mutableConstantsResource->resourceGPUAddress);
	raytracingBuilder.SetBuffer(1, accelerationStructureDesc.vertexBufferAddress);
	raytracingBuilder.SetBuffer(2, accelerationStructureDesc.indexBufferAddress);
	raytracingBuilder.SetTexture(3, whiteroomAlbedo->srvDescriptor.gpuDescriptor);
	raytracingBuilder.SetTexture(4, whiteroomNormal->srvDescriptor.gpuDescriptor);
	raytracingBuilder.SetSampler(0, linearSampler->samplerDescriptor.gpuDescriptor);
	raytracingBuilder.SetShader(libraryDesc);
	lightingObject = raytracingBuilder.Compose(device5, commandList5, renderer->GetBufferManager());

	whiteroomVertexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	whiteroomIndexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	device5->Release();
	commandList5->Release();

	postProcessManager = new SceneEntity::PostProcessManager(commandList, renderer);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadCache(std::filesystem::path filePath, MeshDesc& meshDesc,
	std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData)
{
	std::ifstream meshFile(filePath, std::ios::binary);
	meshFile.read(reinterpret_cast<char*>(&meshDesc), sizeof(MeshDesc));

	auto vertexBufferSize = static_cast<size_t>(meshDesc.verticesNumber) * VertexStride(meshDesc.vertexFormat);
	verticesData.resize(vertexBufferSize);
	meshFile.read(reinterpret_cast<char*>(verticesData.data()), vertexBufferSize);

	auto indexBufferSize = static_cast<size_t>(meshDesc.indicesNumber);
	indexBufferSize *= meshDesc.indexFormat == IndexFormat::UINT16_INDEX ? 2u : 4u;
	indicesData.resize(indexBufferSize);
	meshFile.read(reinterpret_cast<char*>(indicesData.data()), indexBufferSize);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::SaveCache(std::filesystem::path filePath, const MeshDesc& meshDesc,
	const std::vector<uint8_t>& verticesData, const std::vector<uint8_t>& indicesData)
{
	std::ofstream meshFile(filePath, std::ios::binary);
	meshFile.write(reinterpret_cast<const char*>(&meshDesc), sizeof(MeshDesc));
	meshFile.write(reinterpret_cast<const char*>(verticesData.data()), verticesData.size());
	meshFile.write(reinterpret_cast<const char*>(indicesData.data()), indicesData.size());
}
