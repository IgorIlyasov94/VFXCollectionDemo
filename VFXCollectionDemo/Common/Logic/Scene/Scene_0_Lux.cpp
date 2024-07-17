#include "Scene_0_Lux.h"
#include "../SceneEntity/MeshObject.h"
#include "../SceneEntity/VFXLux.h"
#include "../SceneEntity/VFXLuxSparkles.h"
#include "../SceneEntity/VFXLuxDistorters.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace DirectX;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;
using namespace Common::Logic::SceneEntity;

Common::Logic::Scene::Scene_0_Lux::Scene_0_Lux()
	: isLoaded(false), terrain(nullptr), vegetationSystem(nullptr), lightingSystem(nullptr),
	camera(nullptr), postProcessManager(nullptr), mutableConstantsBuffer{}, mutableConstantsId{},
	environmentFloorAlbedoId{}, environmentFloorNormalId{}, vfxAtlasId{}, perlinNoiseId{}, pbrStandardVSId{},
	pbrStandardPSId{}, particleSimulationCSId{}, environmentWorld{}, timer{}, _deltaTime{}, fps(60.0f),
	cpuTimeCounter{}, prevTimePoint{}, frameCounter{}, vfxLux{}, vfxLuxSparkles{}, environmentWallsAlbedoId{},
	environmentWallsNormalId{}, wallsMaterial{}, wallsMesh{}, wallsMeshObject{}, vfxLuxDistorters{}
{
	environmentPosition = float3(0.0f, 0.0f, 0.0f);
	cameraPosition = float3(0.0f, 0.0f, 5.0f);
	windDirection = float3(0.45f, 0.0f, 0.0f);
	windStrength = 4.5f;

	XMStoreFloat3(&cameraLookAt, DirectX::XMLoadFloat3(&environmentPosition));
	cameraLookAt.z = 1.0f;

	cameraUpVector = float3(0.0f, 0.0f, 1.0f);
	auto cameraUpVectorN = DirectX::XMLoadFloat3(&cameraUpVector);
	auto cameraPositionN = DirectX::XMLoadFloat3(&cameraPosition);
	cameraUpVectorN += cameraPositionN;

	XMStoreFloat3(&cameraUpVector, cameraUpVectorN);
}

Common::Logic::Scene::Scene_0_Lux::~Scene_0_Lux()
{
	
}

void Common::Logic::Scene::Scene_0_Lux::Load(Graphics::DirectX12Renderer* renderer)
{
	prevTimePoint = std::chrono::high_resolution_clock::now();

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	auto commandList = renderer->StartCreatingResources();

	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	LoadMeshes(device, commandList, resourceManager);
	CreateConstantBuffers(device, commandList, resourceManager, width, height);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateObjects(commandList, renderer);

	renderer->EndCreatingResources();

	isLoaded = true;
}

void Common::Logic::Scene::Scene_0_Lux::Unload(Graphics::DirectX12Renderer* renderer)
{
	auto resourceManager = renderer->GetResourceManager();

	delete wallsMaterial;

	terrain->Release(resourceManager);
	delete terrain;

	vegetationSystem->Release(resourceManager);
	delete vegetationSystem;

	wallsMesh->Release(resourceManager);
	delete wallsMesh;

	delete wallsMeshObject;

	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Texture>(environmentFloorAlbedoId);
	resourceManager->DeleteResource<Texture>(environmentFloorNormalId);
	resourceManager->DeleteResource<Texture>(environmentWallsAlbedoId);
	resourceManager->DeleteResource<Texture>(environmentWallsNormalId);
	resourceManager->DeleteResource<Texture>(vfxAtlasId);
	resourceManager->DeleteResource<Texture>(perlinNoiseId);

	resourceManager->DeleteResource<Shader>(pbrStandardVSId);
	resourceManager->DeleteResource<Shader>(pbrStandardPSId);

	resourceManager->DeleteResource<Shader>(particleSimulationCSId);

	delete camera;

	postProcessManager->Release(resourceManager);
	delete postProcessManager;

	vfxLux->Release(resourceManager);
	delete vfxLux;

	vfxLuxSparkles->Release(resourceManager);
	delete vfxLuxSparkles;

	vfxLuxDistorters->Release(resourceManager);
	delete vfxLuxDistorters;

	isLoaded = false;
}

void Common::Logic::Scene::Scene_0_Lux::OnResize(Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);
	
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();

	postProcessManager->OnResize(renderer);
}

void Common::Logic::Scene::Scene_0_Lux::Update()
{
	cameraPosition.x = std::cos(timer * 0.2f) * 5.0f;
	cameraPosition.y = std::sin(timer * 0.2f) * 5.0f;
	cameraPosition.z = 3.0f;

	camera->Update(cameraPosition, cameraLookAt, cameraUpVector);

	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();
	mutableConstantsBuffer->time = timer;

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

void Common::Logic::Scene::Scene_0_Lux::Render(ID3D12GraphicsCommandList* commandList)
{
	vfxLuxSparkles->OnCompute(commandList, timer, _deltaTime);
	vfxLuxDistorters->OnCompute(commandList, timer, _deltaTime);

	postProcessManager->SetGBuffer(commandList);

	//wallsMeshObject->Draw(commandList, timer, _deltaTime);
	terrain->Draw(commandList, camera, timer);
	vegetationSystem->Draw(commandList, timer, _deltaTime);

	vfxLux->Draw(commandList, timer, _deltaTime);
	vfxLuxSparkles->Draw(commandList, timer, _deltaTime);

	postProcessManager->SetDistortBuffer(commandList);

	vfxLuxDistorters->Draw(commandList, timer, _deltaTime);

	postProcessManager->Render(commandList);
}

void Common::Logic::Scene::Scene_0_Lux::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	postProcessManager->RenderToBackBuffer(commandList);
}

bool Common::Logic::Scene::Scene_0_Lux::IsLoaded()
{
	return isLoaded;
}

void Common::Logic::Scene::Scene_0_Lux::LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	wallsMesh = new Mesh("Resources\\Meshes\\LuxEnvironmentWalls.obj", device, commandList, resourceManager, false, true);
}

void Common::Logic::Scene::Scene_0_Lux::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera = new Camera(cameraPosition, cameraLookAt, cameraUpVector, FOV_Y, aspectRatio, Z_NEAR, Z_FAR);

	floatN zero{};
	floatN one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	floatN identityQuaternion = XMQuaternionIdentity();
	auto environmentPositionN = DirectX::XMLoadFloat3(&environmentPosition);

	environmentWorld = XMMatrixTransformation(zero, identityQuaternion, one, zero, identityQuaternion, environmentPositionN);
	
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->world = environmentWorld;
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();
	mutableConstantsBuffer->time = 0.0f;
}

void Common::Logic::Scene::Scene_0_Lux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	pbrStandardVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	pbrStandardPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);

	particleSimulationCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ParticleSimulationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::Scene::Scene_0_Lux::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_AlbedoRoughness.dds", textureDesc);
	environmentFloorAlbedoId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentWalls_AlbedoRoughness.dds", textureDesc);
	environmentWallsAlbedoId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_NormalMetalness.dds", textureDesc);
	environmentFloorNormalId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentWalls_NormalMetalness.dds", textureDesc);
	environmentWallsNormalId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\VFXAtlas.dds", textureDesc);
	vfxAtlasId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\PerlinNoise.dds", textureDesc);
	perlinNoiseId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_0_Lux::CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto environmentWallsAlbedoResource = resourceManager->GetResource<Texture>(environmentWallsAlbedoId);
	auto environmentWallsNormalResource = resourceManager->GetResource<Texture>(environmentWallsNormalId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto pbrStandardVS = resourceManager->GetResource<Shader>(pbrStandardVSId);
	auto pbrStandardPS = resourceManager->GetResource<Shader>(pbrStandardPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, environmentWallsAlbedoResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, environmentWallsNormalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_FRONT);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(wallsMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(pbrStandardVS->bytecode);
	materialBuilder.SetPixelShader(pbrStandardPS->bytecode);

	wallsMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::Scene::Scene_0_Lux::CreateObjects(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	lightingSystem = new LightingSystem(renderer);

	LightDesc pointLight{};
	pointLight.position = float3(0.0f, 0.0f, 1.0f);
	pointLight.radius = 0.5f;
	pointLight.color = float3(1.0f, 0.95f, 0.93f);
	pointLight.intensity = 0.2f;
	pointLight.type = LightType::POINT_LIGHT;

	areaLightId = lightingSystem->CreateLight(pointLight);

	pointLight.position = float3(0.0f, 0.0f, 1.5f);
	pointLight.intensity = 0.5f;

	areaLightId = lightingSystem->CreateLight(pointLight);

	pointLight.position = float3(0.0f, 0.0f, 2.0f);
	pointLight.intensity = 0.5f;

	areaLightId = lightingSystem->CreateLight(pointLight);

	pointLight.position = float3(0.0f, 0.0f, 3.0f);
	pointLight.intensity = 1.0f;

	areaLightId = lightingSystem->CreateLight(pointLight);

	LightDesc ambientLight{};
	ambientLight.color = float3(0.3f, 0.4f, 0.5f);
	ambientLight.intensity = 0.0f;
	ambientLight.type = LightType::AMBIENT_LIGHT;

	ambientLightId = lightingSystem->CreateLight(ambientLight);
	
	TerrainDesc terrainDesc{};
	terrainDesc.origin = {};
	terrainDesc.verticesPerWidth = 256u;
	terrainDesc.verticesPerHeight = 256u;
	terrainDesc.size = float3(20.0f, 20.0f, 2.5f);
	terrainDesc.map0Tiling = float2(10.0f, 10.0f);
	terrainDesc.map1Tiling = float2(11.0f, 11.0f);
	terrainDesc.map2Tiling = float2(8.0f, 8.0f);
	terrainDesc.map3Tiling = float2(12.0f, 12.0f);
	terrainDesc.lightConstantBufferId = lightingSystem->GetLightConstantBufferId();
	terrainDesc.terrainFileName = "Resources\\Meshes\\Lux_Terrain.bin";
	terrainDesc.heightMapFileName = "Resources\\Textures\\Terrain\\Lux_HeightMap.dds";
	terrainDesc.blendMapFileName = "Resources\\Textures\\Terrain\\Lux_BlendWeights.dds";
	terrainDesc.map0AlbedoFileName = "Resources\\Textures\\Terrain\\GrassAlbedo.dds";
	terrainDesc.map1AlbedoFileName = "Resources\\Textures\\Terrain\\MossAlbedo.dds";
	terrainDesc.map2AlbedoFileName = "Resources\\Textures\\Terrain\\OreAlbedo.dds";
	terrainDesc.map3AlbedoFileName = "Resources\\Textures\\Terrain\\StoneAlbedo.dds";
	terrainDesc.map0NormalFileName = "Resources\\Textures\\Terrain\\GrassNormal.dds";
	terrainDesc.map1NormalFileName = "Resources\\Textures\\Terrain\\MossNormal.dds";
	terrainDesc.map2NormalFileName = "Resources\\Textures\\Terrain\\OreNormal.dds";
	terrainDesc.map3NormalFileName = "Resources\\Textures\\Terrain\\StoneNormal.dds";

	terrain = new Terrain(commandList, renderer, terrainDesc);
	
	VegetationSystemDesc vegetationSystemDesc{};
	vegetationSystemDesc.smallGrassNumber = 0;
	vegetationSystemDesc.mediumGrassNumber = 10000;
	vegetationSystemDesc.largeGrassNumber = 300;
	vegetationSystemDesc.atlasRows = 4u;
	vegetationSystemDesc.atlasColumns = 4u;
	vegetationSystemDesc.smallGrassSize = float3(0.2f, 0.2f, 0.1f);
	vegetationSystemDesc.mediumGrassSize = float3(0.4f, 0.4f, 0.8f);
	vegetationSystemDesc.largeGrassSize = float3(0.4f, 0.4f, 1.5f);
	vegetationSystemDesc.terrain = terrain;
	vegetationSystemDesc.windDirection = &windDirection;
	vegetationSystemDesc.windStrength = &windStrength;
	vegetationSystemDesc.perlinNoiseTiling = float2(0.1f, 0.1f);
	vegetationSystemDesc.perlinNoiseId = perlinNoiseId;
	vegetationSystemDesc.lightConstantBufferId = lightingSystem->GetLightConstantBufferId();
	vegetationSystemDesc.vegetationCacheFileName = "Resources\\Meshes\\Lux_Vegetation.bin";
	vegetationSystemDesc.vegetationMapFileName = "Resources\\Textures\\Terrain\\Lux_VegetationMap.dds";
	vegetationSystemDesc.albedoMapFileName = "Resources\\Textures\\Terrain\\VegetationAlbedoAtlas.dds";
	vegetationSystemDesc.normalMapFileName = "Resources\\Textures\\Terrain\\VegetationNormalAtlas.dds";

	vegetationSystem = new VegatationSystem(commandList, renderer, vegetationSystemDesc, camera);

	wallsMeshObject = new MeshObject(wallsMesh, wallsMaterial);

	postProcessManager = new SceneEntity::PostProcessManager(commandList, renderer);

	vfxLux = new SceneEntity::VFXLux(commandList, renderer, perlinNoiseId, camera,
		float3(0.0f, 0.0f, 1.25f));

	vfxLuxSparkles = new SceneEntity::VFXLuxSparkles(commandList, renderer,
		perlinNoiseId, vfxAtlasId, particleSimulationCSId, camera);

	vfxLuxDistorters = new SceneEntity::VFXLuxDistorters(commandList, renderer,
		perlinNoiseId, vfxAtlasId, particleSimulationCSId, camera);
}
