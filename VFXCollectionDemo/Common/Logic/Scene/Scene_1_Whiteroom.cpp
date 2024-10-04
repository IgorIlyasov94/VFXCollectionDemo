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
	postProcessManager{}, groupsNumberX(1u), groupsNumberY(1u), accelerationStructureDesc{},
	accelerationStructureCrystalDesc{}, lightIntensity0{}, lightIntensity1{}, quadMesh{}, pointLight0Id{}, pointLight1Id{},
	quadVSId{}, quadPSId{}, texturedQuadMaterial{}, noiseId{}, crystalVertexBufferResource{}, crystalIndexBufferResource{}
{
	cameraPosition = float3(5.5f, 5.5f, 2.5f);

	cameraLookAt = float3(0.0f, 0.0f, 2.5f);

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

	groupsNumberX = width == 0u ? 1u : width;
	groupsNumberY = height == 0u ? 1u : height;

	lightIntensity0 = 3.0f;
	lightIntensity1 = 3.0f;

	CreateLights(renderer);
	LoadMeshes(device, commandList, resourceManager);
	CreateConstantBuffers(device, resourceManager, width, height);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);
	CreateTargets(device, commandList, resourceManager, width, height);
	CreateObjects(device, commandList, renderer, resourceManager);

	quadMesh = postProcessManager->GetFullscreenQuadMesh();

	CreateMaterials(device, resourceManager);

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
	resourceManager->DeleteResource<Buffer>(crystalVertexBufferId);
	resourceManager->DeleteResource<Buffer>(crystalIndexBufferId);
	resourceManager->DeleteResource<Texture>(whiteroomAlbedoId);
	resourceManager->DeleteResource<Texture>(whiteroomNormalId);
	resourceManager->DeleteResource<Texture>(noiseId);

	resourceManager->DeleteResource<RWTexture>(resultTargetId);

	resourceManager->DeleteResource<Shader>(lightingRSId);
	resourceManager->DeleteResource<Shader>(quadVSId);
	resourceManager->DeleteResource<Shader>(quadPSId);

	delete camera;
	
	postProcessManager->Release(resourceManager);
	delete postProcessManager;

	accelerationStructureDesc = {};
	accelerationStructureCrystalDesc = {};

	isLoaded = false;
}

void Common::Logic::Scene::Scene_1_WhiteRoom::OnResize(Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);

	mutableConstantsBuffer->invViewProjection = camera->GetInvViewProjection();

	groupsNumberX = width;
	groupsNumberY = height;

	auto resourceManager = renderer->GetResourceManager();
	resourceManager->DeleteResource<RWTexture>(resultTargetId);

	auto commandList = renderer->StartCreatingResources();
	CreateTargets(renderer->GetDevice(), commandList, resourceManager, width, height);
	renderer->EndCreatingResources();

	postProcessManager->OnResize(renderer);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::Update()
{
	cameraPosition.x = std::abs(std::cos(timer * 0.2f)) * 5.0f;
	cameraPosition.y = std::abs(std::sin(timer * 0.2f)) * 5.0f;
	cameraPosition.z = 4.0f;

	cameraUpVector = float3(0.0f, 0.0f, 1.0f);
	auto cameraUpVectorN = DirectX::XMLoadFloat3(&cameraUpVector);
	auto cameraPositionN = DirectX::XMLoadFloat3(&cameraPosition);
	cameraUpVectorN += cameraPositionN;

	XMStoreFloat3(&cameraUpVector, cameraUpVectorN);

	camera->Update(cameraPosition, cameraLookAt, cameraUpVector);

	//auto& light0Desc = lightingSystem->GetSourceDesc(pointLight0Id);
	//light0Desc.position.x = 4.5f;
	//light0Desc.position.y = std::cos(timer) * 2.0f;
	//light0Desc.position.z = 2.5f + std::sin(timer) * 2.0f;
	//
	//lightingSystem->UpdateSourceDesc(pointLight0Id);
	//
	//auto& light1Desc = lightingSystem->GetSourceDesc(pointLight1Id);
	//light1Desc.position.x = std::cos(timer * 0.9f) * 2.0f;
	//light1Desc.position.y = 4.5f;
	//light1Desc.position.z = 2.5f + std::sin(timer * 0.9f) * 2.0f;
	//
	//lightingSystem->UpdateSourceDesc(pointLight1Id);

	mutableConstantsBuffer->invViewProjection = camera->GetInvViewProjection();
	mutableConstantsBuffer->cameraPosition = camera->GetPosition();

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

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	postProcessManager->SetDepthPrepass(commandList);
	postProcessManager->SetGBuffer(commandList);

	texturedQuadMaterial->Set(commandList);
	quadMesh->Draw(commandList);

	resultTargetResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	postProcessManager->SetDistortBuffer(commandList);

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

	MeshDesc meshDesc{};
	LoadMesh("Resources\\Meshes\\Whiteroom.obj", "Resources\\Meshes\\Whiteroom.objCACHE", vbDesc.data, ibDesc.data, meshDesc);

	vbDesc.dataStride = static_cast<uint32_t>(vbDesc.data.size() / meshDesc.verticesNumber);
	vbDesc.numElements = meshDesc.verticesNumber;
	ibDesc.dataStride = 1u;
	ibDesc.numElements = static_cast<uint32_t>(ibDesc.data.size());

	whiteroomVertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, vbDesc);
	whiteroomIndexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, ibDesc);

	auto vertexBuffer = resourceManager->GetResource<Buffer>(whiteroomVertexBufferId);
	auto indexBuffer = resourceManager->GetResource<Buffer>(whiteroomIndexBufferId);

	whiteroomVertexBufferResource = vertexBuffer->resource;
	whiteroomIndexBufferResource = indexBuffer->resource;

	whiteroomVertexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	whiteroomIndexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	accelerationStructureDesc.vertexBufferAddress = vertexBuffer->resourceGPUAddress;
	accelerationStructureDesc.indexBufferAddress = indexBuffer->resourceGPUAddress;
	accelerationStructureDesc.vertexStride = vbDesc.dataStride;
	accelerationStructureDesc.indexStride = 2u;
	accelerationStructureDesc.verticesNumber = meshDesc.verticesNumber;
	accelerationStructureDesc.indicesNumber = meshDesc.indicesNumber;
	accelerationStructureDesc.isOpaque = true;

	LoadMesh("Resources\\Meshes\\WhiteroomCrystal.obj", "Resources\\Meshes\\WhiteroomCrystal.objCACHE", vbDesc.data, ibDesc.data, meshDesc);

	vbDesc.dataStride = static_cast<uint32_t>(vbDesc.data.size() / meshDesc.verticesNumber);
	vbDesc.numElements = meshDesc.verticesNumber;
	ibDesc.dataStride = 1u;
	ibDesc.numElements = static_cast<uint32_t>(ibDesc.data.size());

	crystalVertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, vbDesc);
	crystalIndexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::BUFFER, ibDesc);

	vertexBuffer = resourceManager->GetResource<Buffer>(crystalVertexBufferId);
	indexBuffer = resourceManager->GetResource<Buffer>(crystalIndexBufferId);

	crystalVertexBufferResource = vertexBuffer->resource;
	crystalIndexBufferResource = indexBuffer->resource;

	crystalVertexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	crystalIndexBufferResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	accelerationStructureCrystalDesc.vertexBufferAddress = vertexBuffer->resourceGPUAddress;
	accelerationStructureCrystalDesc.indexBufferAddress = indexBuffer->resourceGPUAddress;
	accelerationStructureCrystalDesc.vertexStride = vbDesc.dataStride;
	accelerationStructureCrystalDesc.indexStride = 2u;
	accelerationStructureCrystalDesc.verticesNumber = meshDesc.verticesNumber;
	accelerationStructureCrystalDesc.indicesNumber = meshDesc.indicesNumber;
	accelerationStructureCrystalDesc.isOpaque = false;
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
	mutableConstantsBuffer->cameraPosition = camera->GetPosition();
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	lightingRSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\Lighting\\LightingRS.hlsl",
		ShaderType::RAYTRACING_SHADER, ShaderVersion::SM_6_5);

	quadVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PostProcess\\QuadVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);
	quadPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PostProcess\\TexturedPS.hlsl",
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

	DDSLoader::Load("Resources\\Textures\\Noise.dds", textureDesc);
	noiseId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateLights(Graphics::DirectX12Renderer* renderer)
{
	lightingSystem = new LightingSystem(renderer);

	LightDesc pointLight0{};
	pointLight0.position = float3(4.0f, -4.0f, 1.5f);
	pointLight0.color = float3(1.0f, 0.7f, 0.4f);
	pointLight0.intensity = lightIntensity0;
	pointLight0.type = LightType::POINT_LIGHT;

	pointLight0Id = lightingSystem->CreateLight(pointLight0);

	LightDesc pointLight1{};
	pointLight1.position = float3(-4.0f, 1.5f, 2.5f);
	pointLight1.color = float3(0.4f, 0.7f, 1.0f);
	pointLight1.intensity = lightIntensity1;
	pointLight1.type = LightType::POINT_LIGHT;

	pointLight1Id = lightingSystem->CreateLight(pointLight1);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	TextureDesc targetDesc{};
	targetDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
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

void Common::Logic::Scene::Scene_1_WhiteRoom::CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	auto resultTargetTextureResource = resourceManager->GetResource<Texture>(resultTargetId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_POINT_CLAMP);

	auto quadVS = resourceManager->GetResource<Shader>(quadVSId);
	auto quadPS = resourceManager->GetResource<Shader>(quadPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetTexture(0u, resultTargetTextureResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, false, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
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
	auto noise = resourceManager->GetResource<Texture>(noiseId);
	auto resultTarget = resourceManager->GetResource<RWTexture>(resultTargetId);
	auto linearSampler = resourceManager->GetDefaultSampler(device, DefaultFilterSetup::FILTER_BILINEAR_CLAMP);
	auto pointSampler = resourceManager->GetDefaultSampler(device, DefaultFilterSetup::FILTER_POINT_WRAP);

	auto lightingRS = resourceManager->GetResource<Shader>(lightingRSId);

	RaytracingHitGroup hitGroup{};
	hitGroup.hitGroupName = L"TriangleHitGroup";
	hitGroup.closestHitShaderName = L"ClosestHit";

	RaytracingHitGroup shadowHitGroup{};
	shadowHitGroup.hitGroupName = L"ShadowHitGroup";

	RaytracingHitGroup crystalHitGroup{};
	crystalHitGroup.hitGroupName = L"CrystalTriangleHitGroup";
	crystalHitGroup.anyHitShaderName = L"CrystalAnyHit";
	crystalHitGroup.closestHitShaderName = L"CrystalClosestHit";

	RaytracingHitGroup crystalShadowHitGroup{};
	crystalShadowHitGroup.hitGroupName = L"CrystalShadowHitGroup";
	crystalShadowHitGroup.anyHitShaderName = L"CrystalAnyHit_Shadow";

	RaytracingLibraryDesc libraryDesc{};
	libraryDesc.maxRecursionLevel = 3u;
	libraryDesc.dxilLibrary = lightingRS->bytecode;
	libraryDesc.triangleHitGroups.push_back(hitGroup);
	libraryDesc.triangleHitGroups.push_back(shadowHitGroup);
	libraryDesc.triangleHitGroups.push_back(crystalHitGroup);
	libraryDesc.triangleHitGroups.push_back(crystalShadowHitGroup);
	libraryDesc.rayGenerationShaderName = L"RayGeneration";
	libraryDesc.missShaderNames.push_back(L"Miss");
	libraryDesc.missShaderNames.push_back(L"Miss_Shadow");
	libraryDesc.attributeStride = sizeof(BuiltInTriangleIntersectionAttributes);
	libraryDesc.payloadStride = static_cast<uint32_t>(std::max(sizeof(Payload), sizeof(PayloadShadow)));
	
	ID3D12Device5* device5 = nullptr;
	auto hr = device->QueryInterface(IID_PPV_ARGS(&device5));

	ID3D12GraphicsCommandList5* commandList5 = nullptr;
	commandList->QueryInterface(IID_PPV_ARGS(&commandList5));

	whiteroomAlbedo->resource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	whiteroomNormal->resource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	RaytracingObjectBuilder raytracingBuilder{};
	raytracingBuilder.AddAccelerationStructure(accelerationStructureDesc);
	raytracingBuilder.AddAccelerationStructure(accelerationStructureCrystalDesc);
	raytracingBuilder.SetConstantBuffer(0, lightConstantBufferResource->resourceGPUAddress);
	raytracingBuilder.SetConstantBuffer(1, mutableConstantsResource->resourceGPUAddress);
	raytracingBuilder.SetBuffer(1, accelerationStructureDesc.vertexBufferAddress);
	raytracingBuilder.SetBuffer(2, accelerationStructureDesc.indexBufferAddress);
	raytracingBuilder.SetBuffer(3, accelerationStructureCrystalDesc.vertexBufferAddress);
	raytracingBuilder.SetBuffer(4, accelerationStructureCrystalDesc.indexBufferAddress);
	raytracingBuilder.SetTexture(5, whiteroomAlbedo->srvDescriptor.gpuDescriptor);
	raytracingBuilder.SetTexture(6, whiteroomNormal->srvDescriptor.gpuDescriptor);
	raytracingBuilder.SetTexture(7, noise->srvDescriptor.gpuDescriptor);
	raytracingBuilder.SetRWTexture(0, resultTarget->uavDescriptor.gpuDescriptor);
	raytracingBuilder.SetSampler(0, linearSampler->samplerDescriptor.gpuDescriptor);
	raytracingBuilder.SetSampler(1, pointSampler->samplerDescriptor.gpuDescriptor);
	raytracingBuilder.SetShader(libraryDesc);
	lightingObject = raytracingBuilder.Compose(device5, commandList5, renderer->GetBufferManager());

	device5->Release();
	commandList5->Release();

	postProcessManager = new SceneEntity::PostProcessManager(commandList, renderer);
}

void Common::Logic::Scene::Scene_1_WhiteRoom::LoadMesh(std::filesystem::path filePath,
	std::filesystem::path fileCachePath, std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData,
	Graphics::Assets::MeshDesc& desc)
{
	auto loadCache = std::filesystem::exists(fileCachePath);

	if (loadCache)
	{
		auto mainFileTimestamp = std::filesystem::last_write_time(filePath);
		auto cacheFileTimestamp = std::filesystem::last_write_time(fileCachePath);

		loadCache = cacheFileTimestamp >= mainFileTimestamp;
	}

	if (loadCache)
	{
		LoadCache(fileCachePath, desc, verticesData, indicesData);
	}
	else
	{
		OBJLoader::Load(filePath, false, true, desc, verticesData, indicesData);
		SaveCache(fileCachePath, desc, verticesData, indicesData);
	}
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
