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
	particleSystemDesc.attractors[0u].position.x = std::sin(time * 3.0f);
	particleSystemDesc.attractors[1u].position.x = std::sin(time * 3.0f + 0.1f);
	particleSystemDesc.attractors[2u].position.x = std::sin(time * 3.0f + 0.2f);
	particleSystemDesc.attractors[3u].position.x = std::sin(time * 3.0f + 0.3f);
	particleSystemDesc.attractors[4u].position.x = std::sin(time * 3.0f + 0.4f);
	particleSystemDesc.attractors[5u].position.x = std::sin(time * 3.0f + 0.5f);
	particleSystemDesc.attractors[6u].position.x = std::sin(time * 3.0f + 0.6f);
	particleSystemDesc.attractors[7u].position.x = std::sin(time * 3.0f + 0.7f);

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
	sparklesConstants->atlasElementOffset = float2(0.25f, 0.0f);
	sparklesConstants->atlasElementSize = float2(1.0f / 8.0f, 1.0f / 8.0f);
	sparklesConstants->colorIntensity = 0.5f;
	sparklesConstants->padding = {};
}

void Common::Logic::SceneEntity::VFXLuxSparkles::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxSparklesVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ParticleStandardVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxSparklesPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ParticleStandardPS.hlsl",
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

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, sparklesConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, {}, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, sparklesAnimationResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, vfxAtlasResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
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
	particleSystemDesc.minParticleVelocity = float3(0.0f, 0.0f, 1.1f);
	particleSystemDesc.particleDamping = 0.99f;
	particleSystemDesc.maxParticleVelocity = float3(0.0f, 0.0f, 2.25f);
	particleSystemDesc.particleTurbulence = 0.01f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = static_cast<float>(std::numbers::pi * 2.0);
	particleSystemDesc.minRotationSpeed = 0.0f;
	particleSystemDesc.maxRotationSpeed = 0.5f;
	particleSystemDesc.minSize = float2(0.005f, 0.005f);
	particleSystemDesc.maxSize = float2(0.02f, 0.02f);
	particleSystemDesc.minLifeSec = 1.4f;
	particleSystemDesc.maxLifeSec = 3.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 25u;
	particleSystemDesc.maxParticlesNumber = 10000u;
	particleSystemDesc.perlinNoiseId = perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = particleSimulationCSId;

	particleSystemDesc.attractorsNumber = 8u;
	particleSystemDesc.attractors = new ParticleSystemAttractor[particleSystemDesc.attractorsNumber]{};
	particleSystemDesc.attractors[0u] = { float3(0.5f, 0.0f, 0.5f), 10.0f };
	particleSystemDesc.attractors[1u] = { float3(0.5f, 0.0f, 1.0f), 25.0f };
	particleSystemDesc.attractors[2u] = { float3(0.5f, 0.0f, 1.5f), 42.0f };
	particleSystemDesc.attractors[3u] = { float3(0.5f, 0.0f, 2.0f), 55.0f };
	particleSystemDesc.attractors[4u] = { float3(0.5f, 0.0f, 2.5f), 67.0f };
	particleSystemDesc.attractors[5u] = { float3(0.5f, 0.0f, 3.0f), 80.0f };
	particleSystemDesc.attractors[6u] = { float3(0.5f, 0.0f, 3.5f), 99.0f };
	particleSystemDesc.attractors[7u] = { float3(0.5f, 0.0f, 4.0f), 110.0f };

	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	particleSystem = new SceneEntity::ParticleSystem(commandList, renderer, sparklesMaterial, particleSystemDesc);
}
