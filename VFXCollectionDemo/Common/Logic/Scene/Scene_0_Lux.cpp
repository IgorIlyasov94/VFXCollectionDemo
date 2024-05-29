#include "Scene_0_Lux.h"
#include "../SceneEntity/MeshObject.h"
#include "../SceneEntity/VFXLux.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace DirectX;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;
using namespace Common::Logic::SceneEntity;

Common::Logic::Scene::Scene_0_Lux::Scene_0_Lux()
	: isLoaded(false), environmentMaterial(nullptr), environmentMesh(nullptr), environmentMeshObject(nullptr),
	camera(nullptr), postProcessManager(nullptr), mutableConstantsBuffer{}, mutableConstantsId{}, samplerLinearId{},
	environmentFloorAlbedoId{}, environmentFloorNormalId{}, vfxAtlasId{}, pbrStandardVSId{}, pbrStandardPSId{},
	environmentWorld{}, timer{}, fps(60.0f), cpuTimeCounter{}, prevTimePoint{}, frameCounter{}, vfxLux{}
{
	environmentPosition = float3(0.0f, 0.0f, 0.0f);
	cameraPosition = float3(0.0f, 0.0f, 5.0f);

	XMStoreFloat3(&cameraLookAt, DirectX::XMLoadFloat3(&environmentPosition));

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
	CreateSamplers(device, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateObjects();

	postProcessManager = new SceneEntity::PostProcessManager(commandList, renderer);
	//vfxLux = new SceneEntity::VFXLux(commandList, renderer, vfxAtlasId);

	renderer->EndCreatingResources();

	isLoaded = true;
}

void Common::Logic::Scene::Scene_0_Lux::Unload(Graphics::DirectX12Renderer* renderer)
{
	auto resourceManager = renderer->GetResourceManager();

	delete environmentMaterial;

	environmentMesh->Release(resourceManager);
	delete environmentMesh;

	delete environmentMeshObject;

	resourceManager->DeleteResource<ConstantBuffer>(mutableConstantsId);
	resourceManager->DeleteResource<Sampler>(samplerLinearId);
	resourceManager->DeleteResource<Texture>(environmentFloorAlbedoId);
	resourceManager->DeleteResource<Texture>(environmentFloorNormalId);
	
	resourceManager->DeleteResource<Shader>(pbrStandardVSId);
	resourceManager->DeleteResource<Shader>(pbrStandardPSId);

	delete camera;

	postProcessManager->Release(resourceManager);
	delete postProcessManager;

	resourceManager->DeleteResource<Texture>(vfxAtlasId);

	isLoaded = false;
}

void Common::Logic::Scene::Scene_0_Lux::OnResize(Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);
	auto& view = camera->GetView();
	auto& projection = camera->GetProjection();

	mutableConstantsBuffer->viewProjection = view * projection;

	postProcessManager->OnResize(renderer);
}

void Common::Logic::Scene::Scene_0_Lux::Update()
{
	cameraPosition.x = std::cos(timer) * 5.0f;
	cameraPosition.y = std::sin(timer) * 5.0f;
	cameraPosition.z = 5.0f;

	camera->Update(cameraPosition, cameraLookAt, cameraUpVector);

	auto& view = camera->GetView();
	auto& projection = camera->GetProjection();

	mutableConstantsBuffer->viewProjection = view * projection;

	auto cameraDirection = XMLoadFloat3(&cameraLookAt);
	cameraDirection -= XMLoadFloat3(&cameraPosition);
	cameraDirection = XMVector3Normalize(cameraDirection);

	XMStoreFloat4(&mutableConstantsBuffer->cameraDirection, cameraDirection);

	auto currentTimePoint = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimePoint - prevTimePoint);

	timer += 0.2f / fps;

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
	postProcessManager->SetGBuffer(commandList);

	environmentMeshObject->Draw(commandList);

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
	environmentMesh = new Mesh("Resources\\Meshes\\LuxEnvironmentFloor.obj", device, commandList, resourceManager, false, true);
}

void Common::Logic::Scene::Scene_0_Lux::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	camera = new Camera(cameraPosition, cameraLookAt, cameraUpVector, FOV_Y, aspectRatio, Z_NEAR, Z_FAR);

	XMVECTOR zero{};
	XMVECTOR one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR identityQuaternion = XMQuaternionIdentity();
	auto environmentPositionN = DirectX::XMLoadFloat3(&environmentPosition);

	environmentWorld = XMMatrixTransformation(zero, identityQuaternion, one, zero, identityQuaternion, environmentPositionN);
	auto& view = camera->GetView();
	auto& projection = camera->GetProjection();

	std::vector<uint8_t> mutableConstantsBufferTemp;
	mutableConstantsBufferTemp.resize(sizeof(MutableConstants));

	BufferDesc bufferDesc{};
	bufferDesc.data = mutableConstantsBufferTemp;
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->world = environmentWorld;
	mutableConstantsBuffer->viewProjection = view * projection;

	auto cameraDirection = XMLoadFloat3(&cameraLookAt);
	cameraDirection -= XMLoadFloat3(&cameraPosition);
	cameraDirection = XMVector3Normalize(cameraDirection);

	XMStoreFloat4(&mutableConstantsBuffer->cameraDirection, cameraDirection);
}

void Common::Logic::Scene::Scene_0_Lux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	pbrStandardVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	pbrStandardPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::Scene::Scene_0_Lux::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_AlbedoRoughness.dds", textureDesc);
	environmentFloorAlbedoId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_NormalMetalness.dds", textureDesc);
	environmentFloorNormalId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\VFXAtlas.dds", textureDesc);
	vfxAtlasId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::Scene::Scene_0_Lux::CreateSamplers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	auto samplerDesc = Graphics::DirectX12Utilities::CreateSamplerDesc(Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);
	samplerLinearId = resourceManager->CreateSamplerResource(device, samplerDesc);
}

void Common::Logic::Scene::Scene_0_Lux::CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto environmentFloorAlbedoResource = resourceManager->GetResource<Texture>(environmentFloorAlbedoId);
	auto environmentFloorNormalResource = resourceManager->GetResource<Texture>(environmentFloorNormalId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(vfxAtlasId);
	auto samplerLinearResource = resourceManager->GetResource<Sampler>(samplerLinearId);

	auto pbrStandardVS = resourceManager->GetResource<Shader>(pbrStandardVSId);
	auto pbrStandardPS = resourceManager->GetResource<Shader>(pbrStandardPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, environmentFloorAlbedoResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, environmentFloorNormalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R11G11B10_FLOAT);
	materialBuilder.SetGeometryFormat(environmentMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(pbrStandardVS->bytecode);
	materialBuilder.SetPixelShader(pbrStandardPS->bytecode);

	environmentMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::Scene::Scene_0_Lux::CreateObjects()
{
	environmentMeshObject = new MeshObject(environmentMesh, environmentMaterial);
}
