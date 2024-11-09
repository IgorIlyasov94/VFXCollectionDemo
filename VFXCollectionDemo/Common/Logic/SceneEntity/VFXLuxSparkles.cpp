#include "VFXLuxSparkles.h"
#include "../SceneEntity/ParticleSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VFXLuxSparkles::VFXLuxSparkles(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, Camera* camera, const VFXLuxSparklesDesc& desc)
	: firefliesOrigin{}, sparklesOrigin{}
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, desc);
	CreateParticleSystems(commandList, renderer, desc);
}

Common::Logic::SceneEntity::VFXLuxSparkles::~VFXLuxSparkles()
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::Update(float time, float deltaTime)
{
	firefliesConstants->invView = _camera->GetInvView();
	firefliesConstants->viewProjection = _camera->GetViewProjection();
	firefliesConstants->time = time;

	sparklesConstants->invView = _camera->GetInvView();
	sparklesConstants->viewProjection = _camera->GetViewProjection();
	sparklesConstants->time = time;

	firefliesParticleSystem->Update(time, deltaTime);
	sparklesParticleSystem->Update(time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::OnCompute(ID3D12GraphicsCommandList* commandList)
{
	firefliesParticleSystem->OnCompute(commandList);
	sparklesParticleSystem->OnCompute(commandList);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::Draw(ID3D12GraphicsCommandList* commandList)
{
	firefliesParticleSystem->Draw(commandList);
	sparklesParticleSystem->Draw(commandList);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete firefliesMaterial;
	delete sparklesMaterial;

	resourceManager->DeleteResource<ConstantBuffer>(firefliesConstantsId);
	resourceManager->DeleteResource<ConstantBuffer>(sparklesConstantsId);
	resourceManager->DeleteResource<Texture>(sparklesAnimationId);
	resourceManager->DeleteResource<Shader>(vfxLuxSparklesVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxSparklesPSId);

	firefliesParticleSystem->Release(resourceManager);
	delete firefliesParticleSystem;

	sparklesParticleSystem->Release(resourceManager);
	delete sparklesParticleSystem;
}

void Common::Logic::SceneEntity::VFXLuxSparkles::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXSparklesConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	firefliesConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);
	sparklesConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto firefliesConstantsResource = resourceManager->GetResource<ConstantBuffer>(firefliesConstantsId);
	firefliesConstants = reinterpret_cast<VFXSparklesConstants*>(firefliesConstantsResource->resourceCPUAddress);
	firefliesConstants->invView = _camera->GetInvView();
	firefliesConstants->viewProjection = _camera->GetViewProjection();
	firefliesConstants->atlasElementOffset.x = static_cast<float>(FIREFLY_SPRITE_INDEX % VFX_ATLAS_SIZE) / VFX_ATLAS_SIZE;
	firefliesConstants->atlasElementOffset.y = static_cast<float>(FIREFLY_SPRITE_INDEX / VFX_ATLAS_SIZE) / VFX_ATLAS_SIZE;
	firefliesConstants->atlasElementSize = float2(1.0f / VFX_ATLAS_SIZE, 1.0f / VFX_ATLAS_SIZE);
	firefliesConstants->colorIntensity = 7.0f;
	firefliesConstants->perlinNoiseTiling = float3(0.3f, 0.3f, 0.3f);
	firefliesConstants->perlinNoiseScrolling = float3(0.011f, 0.019f, -0.017f);
	firefliesConstants->particleTurbulence = 0.25f;
	firefliesConstants->time = 0.0f;
	firefliesConstants->padding = {};

	auto sparklesConstantsResource = resourceManager->GetResource<ConstantBuffer>(sparklesConstantsId);
	sparklesConstants = reinterpret_cast<VFXSparklesConstants*>(sparklesConstantsResource->resourceCPUAddress);
	sparklesConstants->invView = _camera->GetInvView();
	sparklesConstants->viewProjection = _camera->GetViewProjection();
	sparklesConstants->atlasElementOffset.x = static_cast<float>(SPARKLE_SPRITE_INDEX % VFX_ATLAS_SIZE) / VFX_ATLAS_SIZE;
	sparklesConstants->atlasElementOffset.y = static_cast<float>(SPARKLE_SPRITE_INDEX / VFX_ATLAS_SIZE) / VFX_ATLAS_SIZE;
	sparklesConstants->atlasElementSize = float2(1.0f / VFX_ATLAS_SIZE, 1.0f / VFX_ATLAS_SIZE);
	sparklesConstants->colorIntensity = 5.0f;
	sparklesConstants->perlinNoiseTiling =  float3(0.3f, 0.3f, 0.3f);
	sparklesConstants->perlinNoiseScrolling = float3(0.011f, 0.019f, -0.017f);
	sparklesConstants->particleTurbulence = 0.25f;
	sparklesConstants->time = 0.0f;
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
	Graphics::Resources::ResourceManager* resourceManager, const VFXLuxSparklesDesc& desc)
{
	auto sparklesConstantsResource = resourceManager->GetResource<ConstantBuffer>(sparklesConstantsId);
	auto firefliesConstantsResource = resourceManager->GetResource<ConstantBuffer>(firefliesConstantsId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(desc.vfxAtlasId);
	auto sparklesAnimationResource = resourceManager->GetResource<Texture>(sparklesAnimationId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(desc.perlinNoiseId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_BILINEAR_WRAP);

	auto vfxLuxSparklesVS = resourceManager->GetResource<Shader>(vfxLuxSparklesVSId);
	auto vfxLuxSparklesPS = resourceManager->GetResource<Shader>(vfxLuxSparklesPSId);

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, firefliesConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, {}, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, sparklesAnimationResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(3u, vfxAtlasResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, true, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);

	materialBuilder.SetGeometryFormat(Graphics::VertexFormat::POSITION | Graphics::VertexFormat::TEXCOORD0,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	materialBuilder.SetVertexShader(vfxLuxSparklesVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxSparklesPS->bytecode);

	firefliesMaterial = materialBuilder.ComposeStandard(device);

	materialBuilder.SetConstantBuffer(0u, sparklesConstantsResource->resourceGPUAddress);
	materialBuilder.SetBuffer(0u, {}, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(1u, sparklesAnimationResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(2u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_VERTEX);
	materialBuilder.SetTexture(3u, vfxAtlasResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, true, true);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);

	materialBuilder.SetGeometryFormat(Graphics::VertexFormat::POSITION | Graphics::VertexFormat::TEXCOORD0,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	materialBuilder.SetVertexShader(vfxLuxSparklesVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxSparklesPS->bytecode);

	sparklesMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::CreateParticleSystems(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, const VFXLuxSparklesDesc& desc)
{
	firefliesOrigin = float3(0.0f, 0.0f, 1.2f);
	sparklesOrigin = float3(0.0f, 0.0f, 1.25f);

	ParticleSystemForce fireflyForce{};
	fireflyForce.type = ParticleSystemForceType::ATTRACTOR;
	fireflyForce.strength = -100.0f;
	fireflyForce.nAccelerationCoeff = 1.0f;
	fireflyForce.position = float3(0.0f, 0.0f, 1.25f);

	firefliesForces.push_back(std::move(fireflyForce));
	
	ParticleSystemForce sparkleForce{};
	sparkleForce.type = ParticleSystemForceType::CIRCULAR;
	sparkleForce.strength = 0.5f;
	sparkleForce.nAccelerationCoeff = 0.5f;
	sparkleForce.tAccelerationCoeff = 0.5f;
	sparkleForce.axis = float3(0.0f, 0.0f, 1.0f);
	sparkleForce.position = float3(0.0f, 0.0f, 1.25f);

	sparklesForces.push_back(std::move(sparkleForce));

	ParticleSystemDesc particleSystemDesc{};
	particleSystemDesc.emitterOrigin = &firefliesOrigin;
	particleSystemDesc.emitterRadius = float3(1.0f, 1.0f, 0.8f);
	particleSystemDesc.emitterRadiusOffset = float3(6.0f, 6.0f, 0.0f);
	particleSystemDesc.minParticleVelocity = float3(-0.3f, -0.3f, -0.05f);
	particleSystemDesc.maxParticleVelocity = float3(0.3f, 0.3f, 0.05f);
	particleSystemDesc.particleDamping = 0.995f;
	particleSystemDesc.particleTurbulence = 0.0f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = 0.0f;
	particleSystemDesc.minRotationSpeed = 0.0f;
	particleSystemDesc.maxRotationSpeed = 0.0f;
	particleSystemDesc.minSize = float2(0.3f, 0.3f);
	particleSystemDesc.maxSize = float2(0.7f, 0.7f);
	particleSystemDesc.minLifeSec = 2.4f;
	particleSystemDesc.maxLifeSec = 9.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 25u;
	particleSystemDesc.maxParticlesNumber = desc.firefliesNumber;
	particleSystemDesc.perlinNoiseId = desc.perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = desc.firefliesSimulationCSId;
	particleSystemDesc.hasLightSources = true;
	particleSystemDesc.particleLightBufferId = desc.lightParticleBufferId;
	particleSystemDesc.animationTextureId = sparklesAnimationId;
	particleSystemDesc.maxLightIntensity = MAX_LIGHT_INTENSITY;
	particleSystemDesc.lightRange = LIGHT_RANGE;
	particleSystemDesc.forcesNumber = static_cast<uint32_t>(firefliesForces.size());
	particleSystemDesc.forces = firefliesForces.data();

	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	firefliesParticleSystem = new SceneEntity::ParticleSystem(commandList, renderer, firefliesMaterial, particleSystemDesc);

	particleSystemDesc.emitterOrigin = &sparklesOrigin;
	particleSystemDesc.emitterRadius = float3(0.2f, 0.2f, 0.2f);
	particleSystemDesc.emitterRadiusOffset = float3(0.2f, 0.2f, 0.2f);
	particleSystemDesc.minParticleVelocity = float3(0.0f, 0.0f, 0.0f);
	particleSystemDesc.particleDamping = 0.995f;
	particleSystemDesc.maxParticleVelocity = float3(0.0f, 0.0f, 0.0f);
	particleSystemDesc.particleTurbulence = 0.0f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = 0.0f;
	particleSystemDesc.minRotationSpeed = 0.0f;
	particleSystemDesc.maxRotationSpeed = 0.0f;
	particleSystemDesc.minSize = float2(0.1f, 0.1f);
	particleSystemDesc.maxSize = float2(0.25f, 0.25f);
	particleSystemDesc.minLifeSec = 7.5f;
	particleSystemDesc.maxLifeSec = 10.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 50u;
	particleSystemDesc.maxParticlesNumber = desc.sparklesNumber;
	particleSystemDesc.perlinNoiseId = desc.perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = desc.sparkleSimulationCSId;
	particleSystemDesc.hasLightSources = false;
	particleSystemDesc.particleLightBufferId = desc.lightParticleBufferId;
	particleSystemDesc.animationTextureId = sparklesAnimationId;
	particleSystemDesc.maxLightIntensity = 0.0f;
	particleSystemDesc.lightRange = 0.0f;
	particleSystemDesc.forcesNumber = static_cast<uint32_t>(sparklesForces.size());
	particleSystemDesc.forces = sparklesForces.data();

	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	sparklesParticleSystem = new SceneEntity::ParticleSystem(commandList, renderer, sparklesMaterial, particleSystemDesc);
}
