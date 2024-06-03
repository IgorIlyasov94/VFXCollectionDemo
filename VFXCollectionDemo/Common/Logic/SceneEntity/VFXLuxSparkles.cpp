#include "VFXLuxSparkles.h"
#include "../SceneEntity/ParticleSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VFXLuxSparkles::VFXLuxSparkles(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID vfxAtlasId,
	ResourceID particleSimulationCSId, Camera* camera)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, vfxAtlasId);
	CreateParticleSystems(commandList, renderer, perlinNoiseId, particleSimulationCSId);
}

Common::Logic::SceneEntity::VFXLuxSparkles::~VFXLuxSparkles()
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	particleSystem->OnCompute(commandList, time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	sparklesConstants->invView = _camera->GetInvView();
	sparklesConstants->viewProjection = _camera->GetViewProjection();

	particleSystem->Draw(commandList, time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete sparklesMaterial;

	resourceManager->DeleteResource<ConstantBuffer>(sparklesConstantsId);
	resourceManager->DeleteResource<Texture>(sparklesAnimationId);
	resourceManager->DeleteResource<Shader>(vfxLuxSparklesVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxSparklesPSId);

	particleSystem->Release(resourceManager);
	delete particleSystem;

	delete particleSystemDesc.emitterOrigin;
	delete[] particleSystemDesc.attractors;
}

void Common::Logic::SceneEntity::VFXLuxSparkles::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXSparklesConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	sparklesConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto sparklesConstantsResource = resourceManager->GetResource<ConstantBuffer>(sparklesConstantsId);
	sparklesConstants = reinterpret_cast<VFXSparklesConstants*>(sparklesConstantsResource->resourceCPUAddress);
	sparklesConstants->invView = _camera->GetInvView();
	sparklesConstants->viewProjection = _camera->GetViewProjection();
	sparklesConstants->atlasElementOffset = float2(0.0f, 0.0f);
	sparklesConstants->atlasElementSize = float2(1.0f / 8.0f, 1.0f / 8.0f);
	sparklesConstants->colorIntensity = 1.0f;
	sparklesConstants->padding = {};
}

void Common::Logic::SceneEntity::VFXLuxSparkles::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxSparklesVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxSparklesVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxSparklesPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxSparklesPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::LoadTextures(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxSparklesAnimation.dds", textureDesc);
	sparklesAnimationId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::CreateMaterials(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, Graphics::Resources::ResourceID vfxAtlasId)
{
	auto sparklesConstantsResource = resourceManager->GetResource<ConstantBuffer>(sparklesConstantsId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(vfxAtlasId);
	auto sparklesAnimationResource = resourceManager->GetResource<Texture>(sparklesAnimationId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_BILINEAR_CLAMP);

	auto vfxLuxSparklesVS = resourceManager->GetResource<Shader>(vfxLuxSparklesVSId);
	auto vfxLuxSparklesPS = resourceManager->GetResource<Shader>(vfxLuxSparklesPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, sparklesConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, {}, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, sparklesAnimationResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, vfxAtlasResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);

	materialBuilder.SetGeometryFormat(Graphics::VertexFormat::POSITION | Graphics::VertexFormat::TEXCOORD0,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	materialBuilder.SetVertexShader(vfxLuxSparklesVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxSparklesPS->bytecode);

	sparklesMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::CreateParticleSystems(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID particleSimulationCSId)
{
	particleSystemDesc.emitterOrigin = new float3(0.0f, 0.0f, 0.25f);
	particleSystemDesc.emitterRadius = 0.25f;
	particleSystemDesc.minParticleVelocity = float3(0.0f, 0.0f, 0.1f);
	particleSystemDesc.particleDamping = 0.999f;
	particleSystemDesc.maxParticleVelocity = float3(0.0f, 0.0f, 0.25f);
	particleSystemDesc.particleTurbulence = 0.001f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = static_cast<float>(std::numbers::pi * 2.0);
	particleSystemDesc.minRotationSpeed = 0.01f;
	particleSystemDesc.maxRotationSpeed = 0.05f;
	particleSystemDesc.minSize = float2(0.01f, 0.01f);
	particleSystemDesc.maxSize = float2(0.025f, 0.025f);
	particleSystemDesc.minLifeSec = 0.4f;
	particleSystemDesc.maxLifeSec = 1.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 25u;
	particleSystemDesc.maxParticlesNumber = 1000u;
	particleSystemDesc.perlinNoiseId = perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = particleSimulationCSId;

	particleSystemDesc.attractorsNumber = 2u;
	particleSystemDesc.attractors = new ParticleSystemAttractor[particleSystemDesc.attractorsNumber];
	particleSystemDesc.attractors[0u] = { float3(0.1f, 0.0f, 0.3f), 0.25f };
	particleSystemDesc.attractors[1u] = { float3(0.1f, 0.0f, 0.6f), 0.25f };

	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	particleSystem = new SceneEntity::ParticleSystem(commandList, renderer, sparklesMaterial, particleSystemDesc);
}
