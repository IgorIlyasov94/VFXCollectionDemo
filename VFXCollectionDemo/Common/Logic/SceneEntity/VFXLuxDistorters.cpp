#include "VFXLuxDistorters.h"
#include "../SceneEntity/ParticleSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VFXLuxDistorters::VFXLuxDistorters(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID vfxAtlasId,
	ResourceID particleSimulationCSId, Camera* camera)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, vfxAtlasId, perlinNoiseId);
	CreateParticleSystems(commandList, renderer, perlinNoiseId, particleSimulationCSId);
}

Common::Logic::SceneEntity::VFXLuxDistorters::~VFXLuxDistorters()
{

}

void Common::Logic::SceneEntity::VFXLuxDistorters::OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	particleSystem->OnCompute(commandList, time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	distortersConstants->invView = _camera->GetInvView();
	distortersConstants->viewProjection = _camera->GetViewProjection();
	distortersConstants->time = time;

	particleSystem->Draw(commandList, time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete distortersMaterial;

	resourceManager->DeleteResource<ConstantBuffer>(distortersConstantsId);
	resourceManager->DeleteResource<Texture>(distortersAnimationId);
	resourceManager->DeleteResource<Shader>(vfxLuxDistorterVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxDistorterPSId);

	particleSystem->Release(resourceManager);
	delete particleSystem;

	delete particleSystemDesc.emitterOrigin;
	delete[] particleSystemDesc.attractors;
}

void Common::Logic::SceneEntity::VFXLuxDistorters::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXDistortersConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	distortersConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto distortersConstantsResource = resourceManager->GetResource<ConstantBuffer>(distortersConstantsId);
	distortersConstants = reinterpret_cast<VFXDistortersConstants*>(distortersConstantsResource->resourceCPUAddress);
	distortersConstants->invView = _camera->GetInvView();
	distortersConstants->viewProjection = _camera->GetViewProjection();
	distortersConstants->atlasElementOffset = float2(0.5f, 0.0f);
	distortersConstants->atlasElementSize = float2(1.0f / 8.0f, 1.0f / 8.0f);
	distortersConstants->noiseTiling = float2(0.5f, 0.5f);
	distortersConstants->noiseScrollSpeed = float2(0.12f, 0.3f);
	distortersConstants->time = 0.0f;
	distortersConstants->strength = 0.001f;
	distortersConstants->particleNumber = 2000.0f;
	distortersConstants->padding = {};
}

void Common::Logic::SceneEntity::VFXLuxDistorters::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxDistorterVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxDistorterVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxDistorterPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxDistorterPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::LoadTextures(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxDistortersAnimation.dds", textureDesc);
	distortersAnimationId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::CreateMaterials(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, Graphics::Resources::ResourceID vfxAtlasId,
	Graphics::Resources::ResourceID perlinNoiseId)
{
	auto distorterConstantsResource = resourceManager->GetResource<ConstantBuffer>(distortersConstantsId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(vfxAtlasId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto distorterAnimationResource = resourceManager->GetResource<Texture>(distortersAnimationId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto vfxLuxDistorterVS = resourceManager->GetResource<Shader>(vfxLuxDistorterVSId);
	auto vfxLuxDistorterPS = resourceManager->GetResource<Shader>(vfxLuxDistorterPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, distorterConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, {}, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, distorterAnimationResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, vfxAtlasResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(3u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_ADDITIVE));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16_FLOAT);

	materialBuilder.SetGeometryFormat(Graphics::VertexFormat::POSITION | Graphics::VertexFormat::TEXCOORD0,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	materialBuilder.SetVertexShader(vfxLuxDistorterVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxDistorterPS->bytecode);

	distortersMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::CreateParticleSystems(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID particleSimulationCSId)
{
	particleSystemDesc.emitterOrigin = new float3(0.0f, 0.0f, 0.0f);
	particleSystemDesc.emitterRadius = 0.0f;
	particleSystemDesc.minParticleVelocity = float3(0.0f, 0.0f, 0.0f);
	particleSystemDesc.particleDamping = 0.9f;
	particleSystemDesc.maxParticleVelocity = float3(0.0f, 0.0f, 15.5f);
	particleSystemDesc.particleTurbulence = 0.05f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = static_cast<float>(std::numbers::pi * 2.0);
	particleSystemDesc.minRotationSpeed = 0.0f;
	particleSystemDesc.maxRotationSpeed = 0.9f;
	particleSystemDesc.minSize = float2(0.7f, 0.7f);
	particleSystemDesc.maxSize = float2(1.8f, 2.8f);
	particleSystemDesc.minLifeSec = 2.4f;
	particleSystemDesc.maxLifeSec = 6.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 25u;
	particleSystemDesc.maxParticlesNumber = 2000u;
	particleSystemDesc.perlinNoiseId = perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = particleSimulationCSId;

	particleSystemDesc.attractorsNumber = 0u;
	particleSystemDesc.attractors = new ParticleSystemAttractor[particleSystemDesc.attractorsNumber]{};

	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	particleSystem = new SceneEntity::ParticleSystem(commandList, renderer, distortersMaterial, particleSystemDesc);
}
