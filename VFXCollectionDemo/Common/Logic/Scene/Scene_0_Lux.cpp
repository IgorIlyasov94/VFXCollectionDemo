#include "Scene_0_Lux.h"
#include "../SceneEntity/MeshObject.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace DirectX;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;
using namespace Common::Logic::SceneEntity;

Common::Logic::Scene::Scene_0_Lux::Scene_0_Lux()
	: isLoaded(false), environmentMaterial(nullptr), environmentMesh(nullptr), environmentMeshObject(nullptr),
	camera(nullptr), viewport{}, scissorRectangle{}, mutableConstantsBuffer{}, mutableConstantsId{},
	samplerLinearId{}, environmentFloorAlbedoId{}, environmentFloorNormalId{}, pbrStandardVSId{},
	pbrStandardPSId{}, environmentWorld{}, timer{}, fps(60.0f), cpuTimeCounter{}, prevTimePoint{},
	frameCounter{}
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

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRectangle.left = 0;
	scissorRectangle.top = 0;
	scissorRectangle.right = width;
	scissorRectangle.bottom = height;

	environmentMesh = new Mesh("Resources\\Meshes\\LuxEnvironmentFloor.obj", device, commandList, resourceManager, false, true);
	auto vertexFormat = environmentMesh->GetDesc().vertexFormat;

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
	
	pbrStandardVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);
	pbrStandardPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\PBRStandardPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);

	auto pbrStandardVS = resourceManager->GetResource<Shader>(pbrStandardVSId);
	auto pbrStandardPS = resourceManager->GetResource<Shader>(pbrStandardPSId);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->world = environmentWorld;
	mutableConstantsBuffer->viewProjection = view * projection;
	
	auto cameraDirection = XMLoadFloat3(&cameraLookAt);
	cameraDirection -= XMLoadFloat3(&cameraPosition);
	cameraDirection = XMVector3Normalize(cameraDirection);

	XMStoreFloat4(&mutableConstantsBuffer->cameraDirection, cameraDirection);

	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_AlbedoRoughness.dds", textureDesc);
	environmentFloorAlbedoId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	DDSLoader::Load("Resources\\Textures\\LuxEnvironmentFloor_NormalMetalness.dds", textureDesc);
	environmentFloorNormalId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	auto samplerDesc = Graphics::DirectX12Utilities::CreateSamplerDesc(Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);
	samplerLinearId = resourceManager->CreateSamplerResource(device, samplerDesc);

	auto environmentFloorAlbedoResource = resourceManager->GetResource<Texture>(environmentFloorAlbedoId);
	auto environmentFloorNormalResource = resourceManager->GetResource<Texture>(environmentFloorNormalId);
	auto samplerLinearResource = resourceManager->GetResource<Sampler>(samplerLinearId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, environmentFloorAlbedoResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, environmentFloorNormalResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, true);
	materialBuilder.SetRenderTargetFormat(0u, renderer->BACK_BUFFER_FORMAT);
	materialBuilder.SetGeometryFormat(vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(pbrStandardVS->bytecode);
	materialBuilder.SetPixelShader(pbrStandardPS->bytecode);

	environmentMaterial = materialBuilder.ComposeStandard(device);
	environmentMeshObject = new MeshObject(environmentMesh, environmentMaterial);

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

	delete camera;

	isLoaded = false;
}

void Common::Logic::Scene::Scene_0_Lux::OnResize(uint32_t newWidth, uint32_t newHeight)
{
	viewport.Width = static_cast<float>(newWidth);
	viewport.Height = static_cast<float>(newHeight);

	scissorRectangle.right = newWidth;
	scissorRectangle.bottom = newHeight;

	auto aspectRatio = static_cast<float>(newWidth) / static_cast<float>(newHeight);

	camera->UpdateProjection(FOV_Y, aspectRatio, Z_NEAR, Z_FAR);
	auto& view = camera->GetView();
	auto& projection = camera->GetProjection();

	mutableConstantsBuffer->viewProjection = view * projection;
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
	commandList->RSSetViewports(1u, &viewport);
	commandList->RSSetScissorRects(1u, &scissorRectangle);
}

void Common::Logic::Scene::Scene_0_Lux::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	environmentMeshObject->Draw(commandList);
}

bool Common::Logic::Scene::Scene_0_Lux::IsLoaded()
{
	return isLoaded;
}
