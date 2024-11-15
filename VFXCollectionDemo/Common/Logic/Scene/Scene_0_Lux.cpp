#include "Scene_0_Lux.h"
#include "../SceneEntity/MeshObject.h"
#include "../SceneEntity/VFXLux.h"
#include "../SceneEntity/VFXLuxSparkles.h"
#include "../SceneEntity/VFXLuxDistorters.h"
#include "../../Graphics/Assets/MaterialBuilder.h"
#include "../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../Graphics/Assets/Generators/NoiseGenerator.h"
#include "../../Graphics/Assets/Generators/TurbulenceMapGenerator.h"

using namespace DirectX;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;
using namespace Graphics::Assets::Generators;
using namespace Common::Logic::SceneEntity;

Common::Logic::Scene::Scene_0_Lux::Scene_0_Lux()
	: isLoaded(false), terrain(nullptr), vegetationSystem(nullptr), lightingSystem(nullptr),
	camera(nullptr), postProcessManager(nullptr), mutableConstantsBuffer{}, mutableConstantsId{},
	vfxAtlasId{}, perlinNoiseId{}, particleSimulationCSId{}, environmentWorld{}, timer{},
	_deltaTime{}, fps(60.0f), cpuTimeCounter{}, prevTimePoint{}, frameCounter{}, vfxLux{},
	vfxLuxSparkles{}, vfxLuxDistorters{}, areaLightId{}, ambientLightId{}, depthPassVSId{},
	depthCubePassVSId{}, depthCubePassGSId{}, depthPassMaterial{}, depthCubePassMaterial{},
	lightParticleBufferId{}, particleLightSimulationCSId{}, renderSize{}, turbulenceMapId{},
	volumeNoiseId{}
{
	environmentPosition = float3(0.0f, 0.0f, 0.0f);
	cameraPosition = float3(0.0f, 0.0f, 5.0f);
	windDirection = float3(0.45f, 0.0f, 0.0f);
	windStrength = 2.5f;

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
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	auto commandList = renderer->StartCreatingResources();

	renderSize.x = renderer->GetWidth();
	renderSize.y = renderer->GetHeight();

	CreateLights(commandList, renderer);

	CreateConstantBuffers(device, resourceManager, renderSize.x, renderSize.y);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);
	CreateTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateObjects(commandList, renderer);

	renderer->EndCreatingResources();

	prevTimePoint = std::chrono::high_resolution_clock::now();

	isLoaded = true;
}

void Common::Logic::Scene::Scene_0_Lux::Unload(Graphics::DirectX12Renderer* renderer)
{
	auto resourceManager = renderer->GetResourceManager();

	if (depthPassMaterial != nullptr)
		delete depthPassMaterial;

	if (depthCubePassMaterial != nullptr)
		delete depthCubePassMaterial;

	delete lightingSystem;

	terrain->Release(resourceManager);
	delete terrain;

	vegetationSystem->Release(resourceManager);
	delete vegetationSystem;

	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Texture>(vfxAtlasId);
	resourceManager->DeleteResource<Texture>(perlinNoiseId);
	resourceManager->DeleteResource<Texture>(volumeNoiseId);
	resourceManager->DeleteResource<Texture>(turbulenceMapId);

	resourceManager->DeleteResource<Shader>(depthCubePassVSId);
	resourceManager->DeleteResource<Shader>(depthCubePassGSId);

	resourceManager->DeleteResource<Shader>(particleSimulationCSId);
	resourceManager->DeleteResource<Shader>(particleLightSimulationCSId);

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
	renderSize.x = renderer->GetWidth();
	renderSize.y = renderer->GetHeight();

	auto aspectRatio = static_cast<float>(renderSize.x) / static_cast<float>(renderSize.y);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);
	
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();

	postProcessManager->OnResize(renderer);
}

void Common::Logic::Scene::Scene_0_Lux::Update()
{
	cameraPosition.x = std::cos(timer * 0.2f) * 5.0f;
	cameraPosition.y = std::sin(timer * 0.2f) * 5.0f;
	cameraPosition.z = 3.0f;

	if constexpr (FSR_ENABLED)
	{
		float2 jitter{};
		postProcessManager->UpdateFSR(jitter);

		camera->AddJitter(jitter, float2(static_cast<float>(renderSize.x), static_cast<float>(renderSize.y)));
	}

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

	vfxLux->Update(timer, _deltaTime);
	vfxLuxSparkles->Update(timer, _deltaTime);
	vfxLuxDistorters->Update(timer, _deltaTime);
	vegetationSystem->Update(timer, _deltaTime);

	float t = std::sin(timer * 1.2f) * 0.5f + 0.5f;

	auto& areaLightDesc = lightingSystem->GetSourceDesc(areaLightId);
	areaLightDesc.intensity = AREA_LIGHT_INTENSITY + t * 2.0f;
	//areaLightDesc.color = float3(std::lerp(0.8f, 1.0f, t), 0.5f, std::lerp(0.6f, 0.4f, t));
	areaLightDesc.range = AREA_LIGHT_RANGE + t * 2.0f;

	lightingSystem->UpdateSourceDesc(areaLightId);

	auto& ambientLightDesc = lightingSystem->GetSourceDesc(ambientLightId);
	ambientLightDesc.intensity = AMBIENT_LIGHT_INTENSITY + std::pow(std::max(std::sin(timer * 0.4f), 0.0f), 120.0f) * 0.05f;

	lightingSystem->UpdateSourceDesc(ambientLightId);

	terrain->Update(camera, timer);
}

void Common::Logic::Scene::Scene_0_Lux::RenderShadows(ID3D12GraphicsCommandList* commandList)
{
	lightingSystem->BeforeStartRenderShadowMaps(commandList);
	lightingSystem->StartRenderShadowMap(areaLightId, commandList);

	auto& areaLightDesc = lightingSystem->GetSourceDesc(areaLightId);

	terrain->DrawShadowsCube(commandList, areaLightDesc.GetLightMatrixStartIndex());
	vegetationSystem->DrawShadowsCube(commandList, areaLightDesc.GetLightMatrixStartIndex());

	lightingSystem->EndRenderShadowMaps(commandList);
}

void Common::Logic::Scene::Scene_0_Lux::Render(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	vfxLuxSparkles->OnCompute(commandList);
	vfxLuxDistorters->OnCompute(commandList);

	if constexpr (DEPTH_PREPASS_ENABLED)
	{
		postProcessManager->SetDepthPrepass(commandList);

		terrain->DrawDepthPrepass(commandList);
		vegetationSystem->DrawDepthPrepass(commandList);
	}

	postProcessManager->SetGBuffer(commandList);

	terrain->Draw(commandList);
	vegetationSystem->Draw(commandList);

	lightingSystem->EndUsingShadowMaps(commandList);

	vfxLux->Draw(commandList);
	vfxLuxSparkles->Draw(commandList);

	if constexpr (FSR_ENABLED || MOTION_BLUR_ENABLED)
	{
		postProcessManager->SetMotionBuffer(commandList);

		vfxLuxDistorters->Draw(commandList);
	}

	postProcessManager->Render(commandList, renderer, _deltaTime);
}

void Common::Logic::Scene::Scene_0_Lux::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	postProcessManager->RenderToBackBuffer(commandList);
}

bool Common::Logic::Scene::Scene_0_Lux::IsLoaded()
{
	return isLoaded;
}

void Common::Logic::Scene::Scene_0_Lux::CreateConstantBuffers(ID3D12Device* device,
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

	mutableConstantsId = resourceManager->CreateBufferResource(device, nullptr, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->world = environmentWorld;
	mutableConstantsBuffer->viewProjection = camera->GetViewProjection();
	mutableConstantsBuffer->cameraDirection = camera->GetDirection();
	mutableConstantsBuffer->time = 0.0f;
}

void Common::Logic::Scene::Scene_0_Lux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	std::wstringstream lightMatricesNumberStream;
	lightMatricesNumberStream << lightingSystem->GetLightMatricesNumber();
	std::wstring lightMatricesNumberString;
	lightMatricesNumberStream >> lightMatricesNumberString;

	std::vector<DxcDefine> defines;
	defines.push_back({ L"LIGHT_MATRICES_NUMBER", lightMatricesNumberString.c_str() });

	particleSimulationCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ParticleSimulationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	particleLightSimulationCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ParticleLightSimulationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	depthPassVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\DepthPassVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5, defines);

	depthCubePassVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\DepthCubePassVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	depthCubePassGSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\DepthCubePassGS.hlsl",
		ShaderType::GEOMETRY_SHADER, ShaderVersion::SM_6_5, defines);
}

void Common::Logic::Scene::Scene_0_Lux::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\VFXAtlas.dds", textureDesc);
	vfxAtlasId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\PerlinNoise.dds", textureDesc);
	perlinNoiseId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_0_Lux::CreateTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};

	std::filesystem::path fileName("Resources\\Textures\\FogMap.dds");

	if (std::filesystem::exists(fileName))
		DDSLoader::Load(fileName, textureDesc);
	else
	{
		NoiseGenerator noiseGenerator{};

		std::vector<floatN> noiseData;
		noiseGenerator.Generate(NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, float3(4.0f, 4.0f, 4.0f), noiseData);

		DDSSaveDesc ddsSaveDesc{};
		ddsSaveDesc.width = NOISE_SIZE_X;
		ddsSaveDesc.height = NOISE_SIZE_Y;
		ddsSaveDesc.depth = NOISE_SIZE_Z;
		ddsSaveDesc.targetFormat = DDSFormat::R8G8B8A8_UNORM;
		ddsSaveDesc.dimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		DDSLoader::Save(fileName, ddsSaveDesc, noiseData);
		DDSLoader::Load(fileName, textureDesc);
	}

	volumeNoiseId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	fileName = "Resources\\Textures\\TurbulenceMap.dds";

	if (std::filesystem::exists(fileName))
		DDSLoader::Load(fileName, textureDesc);
	else
	{
		TurbulenceMapGenerator turbulenceMapGenerator{};

		std::vector<floatN> turbulenceData;
		turbulenceMapGenerator.Generate(NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, float3(4.0f, 4.0f, 4.0f), turbulenceData);

		DDSSaveDesc ddsSaveDesc{};
		ddsSaveDesc.width = NOISE_SIZE_X;
		ddsSaveDesc.height = NOISE_SIZE_Y;
		ddsSaveDesc.depth = NOISE_SIZE_Z;
		ddsSaveDesc.targetFormat = DDSFormat::R8G8B8A8_UNORM;
		ddsSaveDesc.dimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		DDSLoader::Save(fileName, ddsSaveDesc, turbulenceData);
		DDSLoader::Load(fileName, textureDesc);
	}

	turbulenceMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_0_Lux::CreateLights(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer)
{
	lightingSystem = new LightingSystem(renderer);

	LightDesc areaLight{};
	areaLight.position = float3(0.0f, 0.0f, 1.5f);
	areaLight.radius = 0.05f;
	areaLight.color = LIGHT_COLOR;
	areaLight.intensity = 0.0f;
	areaLight.type = LightType::AREA_LIGHT;
	areaLight.range = AREA_LIGHT_RANGE;
	areaLight.castShadows = true;

	areaLightId = lightingSystem->CreateLight(areaLight);

	LightDesc ambientLight{};
	ambientLight.color = AMBIENT_COLOR;
	ambientLight.intensity = 0.0f;
	ambientLight.type = LightType::AMBIENT_LIGHT;

	ambientLightId = lightingSystem->CreateLight(ambientLight);

	lightParticleBufferId = lightingSystem->CreateLightParticleBuffer(commandList, FIREFLIES_NUMBER);
}

void Common::Logic::Scene::Scene_0_Lux::CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto lightMatricesConstantsResource = resourceManager->GetResource<ConstantBuffer>(lightingSystem->GetLightMatricesConstantBufferId());
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto depthCubePassVS = resourceManager->GetResource<Shader>(depthCubePassVSId);
	auto depthCubePassGS = resourceManager->GetResource<Shader>(depthCubePassGSId);

	auto terrainVertexFormat = Graphics::VertexFormat::POSITION |
		Graphics::VertexFormat::NORMAL |
		Graphics::VertexFormat::TANGENT |
		Graphics::VertexFormat::TEXCOORD0;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetRootConstants(0u, 17u);
	materialBuilder.SetConstantBuffer(1u, lightMatricesConstantsResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_GEOMETRY);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_FRONT);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetGeometryFormat(terrainVertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(depthCubePassVS->bytecode);
	materialBuilder.SetGeometryShader(depthCubePassGS->bytecode);

	depthCubePassMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::Scene::Scene_0_Lux::CreateObjects(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	std::vector<DxcDefine> shaderDefines;
	shaderDefines.push_back({ L"AREA_LIGHT", nullptr });

	if constexpr (FSR_ENABLED)
		shaderDefines.push_back({ L"FSR", nullptr });

	std::wstringstream lightParticleNumberStream;

	if constexpr (USING_PARTICLE_LIGHT)
		lightParticleNumberStream << FIREFLIES_NUMBER;
	else
		lightParticleNumberStream << 0u;

	std::wstring lightParticleNumberString;
	lightParticleNumberStream >> lightParticleNumberString;

	shaderDefines.push_back({ L"PARTICLE_LIGHT_SOURCE_NUMBER", lightParticleNumberString.c_str() });

	auto& areaLightDesc = lightingSystem->GetSourceDesc(areaLightId);

	SceneEntity::RenderingScheme renderingScheme{};
	renderingScheme.enableFSR = FSR_ENABLED;
	renderingScheme.enableDepthPrepass = DEPTH_PREPASS_ENABLED;
	renderingScheme.enableMotionBlur = MOTION_BLUR_ENABLED;
	renderingScheme.enableVolumetricFog = VOLUMETRIC_FOG_ENABLED;
	renderingScheme.useParticleLight = USING_PARTICLE_LIGHT;
	renderingScheme.whiteCutoff = WHITE_CUTOFF;
	renderingScheme.brightThreshold = BRIGHT_THRESHOLD;
	renderingScheme.bloomIntensity = BLOOM_INTENSITY;
	renderingScheme.motionBlurThreshold = MOTION_BLUR_THRESHOLD;
	renderingScheme.colorGrading = COLOR_GRADING;
	renderingScheme.colorGradingFactor = COLOR_GRADING_FACTOR;
	renderingScheme.fogDistanceFalloffStart = FOG_DISTANCE_FALLOFF_START;
	renderingScheme.fogDistanceFalloffLength = FOG_DISTANCE_FALLOFF_LENGTH;
	renderingScheme.fogTiling = FOG_TILING;
	renderingScheme.fogMapId = volumeNoiseId;
	renderingScheme.turbulenceMapId = turbulenceMapId;
	renderingScheme.windDirection = &windDirection;
	renderingScheme.windStrength = &windStrength;

	TerrainDesc terrainDesc{};
	terrainDesc.origin = {};
	terrainDesc.verticesPerWidth = 256u;
	terrainDesc.verticesPerHeight = 256u;
	terrainDesc.size = float3(20.0f, 20.0f, 1.0f);
	terrainDesc.map0Tiling = float2(4.0f, 4.0f);
	terrainDesc.map1Tiling = float2(4.0f, 4.0f);
	terrainDesc.map2Tiling = float2(4.0f, 4.0f);
	terrainDesc.map3Tiling = float2(4.0f, 4.0f);
	terrainDesc.lightConstantBufferId = lightingSystem->GetLightConstantBufferId();
	terrainDesc.hasParticleLighting = USING_PARTICLE_LIGHT;
	terrainDesc.hasDepthPrepass = DEPTH_PREPASS_ENABLED;
	terrainDesc.outputVelocity = renderingScheme.enableFSR || renderingScheme.enableMotionBlur;
	terrainDesc.lightParticleBufferId = lightParticleBufferId;
	terrainDesc.shaderDefines = &shaderDefines;
	terrainDesc.shadowMapIds.push_back(areaLightDesc.GetShadowMapId());
	terrainDesc.materialDepthPass = depthPassMaterial;
	terrainDesc.materialDepthCubePass = depthCubePassMaterial;
	terrainDesc.terrainFileName = "Resources\\Meshes\\Lux_Terrain.bin";
	terrainDesc.heightMapFileName = "Resources\\Textures\\Terrain\\Lux_HeightMap.dds";
	terrainDesc.blendMapFileName = "Resources\\Textures\\Terrain\\Lux_BlendWeights.dds";
	terrainDesc.map0AlbedoFileName = "Resources\\Textures\\Terrain\\GrassAlbedo.dds";
	terrainDesc.map1AlbedoFileName = "Resources\\Textures\\Terrain\\MossAlbedo.dds";
	terrainDesc.map2AlbedoFileName = "Resources\\Textures\\Terrain\\GroundAlbedo.dds";
	terrainDesc.map3AlbedoFileName = "Resources\\Textures\\Terrain\\GroundRootsAlbedo.dds";
	terrainDesc.map0NormalFileName = "Resources\\Textures\\Terrain\\GrassNormal.dds";
	terrainDesc.map1NormalFileName = "Resources\\Textures\\Terrain\\MossNormal.dds";
	terrainDesc.map2NormalFileName = "Resources\\Textures\\Terrain\\GroundNormal.dds";
	terrainDesc.map3NormalFileName = "Resources\\Textures\\Terrain\\GroundRootsNormal.dds";

	terrain = new Terrain(commandList, renderer, terrainDesc);
	
	VegetationSystemDesc vegetationSystemDesc{};
	vegetationSystemDesc.atlasRows = 4u;
	vegetationSystemDesc.atlasColumns = 4u;
	vegetationSystemDesc.grassSizeMin = float3(0.4f, 0.4f, 0.06f);
	vegetationSystemDesc.grassSizeMax = float3(0.8f, 0.8f, 0.5f);
	vegetationSystemDesc.hasDepthPrepass = DEPTH_PREPASS_ENABLED;
	vegetationSystemDesc.hasDepthPass = false;
	vegetationSystemDesc.hasDepthCubePass = true;
	vegetationSystemDesc.hasParticleLighting = USING_PARTICLE_LIGHT;
	vegetationSystemDesc.outputVelocity = renderingScheme.enableFSR || renderingScheme.enableMotionBlur;
	vegetationSystemDesc.lightParticleBufferId = lightParticleBufferId;
	vegetationSystemDesc.lightMatricesNumber = lightingSystem->GetLightMatricesNumber();
	vegetationSystemDesc.terrain = terrain;
	vegetationSystemDesc.windDirection = &windDirection;
	vegetationSystemDesc.windStrength = &windStrength;
	vegetationSystemDesc.perlinNoiseTiling = float2(2.5f, 2.5f);
	vegetationSystemDesc.perlinNoiseId = perlinNoiseId;
	vegetationSystemDesc.lightConstantBufferId = lightingSystem->GetLightConstantBufferId();
	vegetationSystemDesc.lightMatricesConstantBufferId = lightingSystem->GetLightMatricesConstantBufferId();
	vegetationSystemDesc.shadowMapIds.push_back(areaLightDesc.GetShadowMapId());
	vegetationSystemDesc.shaderDefines = &shaderDefines;
	vegetationSystemDesc.vegetationCacheFileName = "Resources\\Meshes\\Lux_Vegetation.bin";
	vegetationSystemDesc.vegetationMapFileName = "Resources\\Textures\\Terrain\\Lux_VegetationMap.dds";
	vegetationSystemDesc.albedoMapFileName = "Resources\\Textures\\Terrain\\VegetationAlbedoAtlas.dds";
	vegetationSystemDesc.normalMapFileName = "Resources\\Textures\\Terrain\\VegetationNormalAtlas.dds";

	VegetationSystemTable grassTableElement{};
	grassTableElement.grassSizeMin = float3(0.1f, 0.1f, 0.2f);
	grassTableElement.grassSizeMax = float3(0.3f, 0.3f, 0.6f);
	grassTableElement.windInfluence = 0.8f;

	vegetationSystemDesc.grassTable[9u] = grassTableElement;

	grassTableElement.grassSizeMin = float3(0.2f, 0.2f, 0.2f);
	grassTableElement.grassSizeMax = float3(0.4f, 0.4f, 0.6f);
	grassTableElement.windInfluence = 0.1f;

	vegetationSystemDesc.grassTable[14u] = grassTableElement;

	vegetationSystem = new VegatationSystem(commandList, renderer, vegetationSystemDesc, camera);

	postProcessManager = new SceneEntity::PostProcessManager(commandList, renderer, camera,
		lightingSystem, shaderDefines, renderingScheme);

	vfxLux = new SceneEntity::VFXLux(commandList, renderer, perlinNoiseId, volumeNoiseId, turbulenceMapId,
		vfxAtlasId, camera, float3(0.0f, 0.0f, 1.25f));

	SceneEntity::VFXLuxSparklesDesc sparklesDesc{};
	sparklesDesc.firefliesNumber = FIREFLIES_NUMBER;
	sparklesDesc.sparklesNumber = SPARKLES_NUMBER;
	sparklesDesc.perlinNoiseId = perlinNoiseId;
	sparklesDesc.vfxAtlasId = vfxAtlasId;
	sparklesDesc.lightParticleBufferId = lightParticleBufferId;
	sparklesDesc.firefliesSimulationCSId = particleLightSimulationCSId;
	sparklesDesc.sparkleSimulationCSId = particleSimulationCSId;

	vfxLuxSparkles = new SceneEntity::VFXLuxSparkles(commandList, renderer, camera, sparklesDesc);

	vfxLuxDistorters = new SceneEntity::VFXLuxDistorters(commandList, renderer,
		perlinNoiseId, vfxAtlasId, particleSimulationCSId, camera);
}
