#include "VFXLuxSparkles.h"
#include "../SceneEntity/ParticleSystem.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::VFXLuxSparkles::VFXLuxSparkles(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID vfxAtlasId,
	Graphics::Resources::ResourceID lightParticleBufferId, ResourceID particleSimulationCSId,
	uint32_t maxParticleNumber, Camera* camera)
	: particleSystemDesc{}
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, perlinNoiseId, vfxAtlasId);

	CreateParticleSystems(commandList, renderer, perlinNoiseId, particleSimulationCSId,
		lightParticleBufferId, maxParticleNumber);
}

Common::Logic::SceneEntity::VFXLuxSparkles::~VFXLuxSparkles()
{

}

void Common::Logic::SceneEntity::VFXLuxSparkles::Update(float time, float deltaTime)
{
	
	sparklesConstants->invView = _camera->GetInvView();
	sparklesConstants->viewProjection = _camera->GetViewProjection();
	sparklesConstants->time = time;

	//particleSystemDesc.forces[0u].axis = _camera->GetDirection();
	//particleSystemDesc.forces[0u].strength = -std::pow(std::max(std::sin(time * 2.8f), 0.0f), 60.0f) * 350.0f - 0.2f;
	
	particleSystem->Update(time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxSparkles::OnCompute(ID3D12GraphicsCommandList* commandList)
{
	particleSystem->OnCompute(commandList);
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
	particleSystem->Draw(commandList);
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
	delete[] particleSystemDesc.forces;
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
	sparklesConstants->atlasElementOffset = float2(0.25f, 0.125f);
	sparklesConstants->atlasElementSize = float2(1.0f / 8.0f, 1.0f / 8.0f);
	sparklesConstants->colorIntensity = 7.0f;
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
	Graphics::Resources::ResourceManager* resourceManager, Graphics::Resources::ResourceID perlinNoiseId,
	Graphics::Resources::ResourceID vfxAtlasId)
{
	auto sparklesConstantsResource = resourceManager->GetResource<ConstantBuffer>(sparklesConstantsId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(vfxAtlasId);
	auto sparklesAnimationResource = resourceManager->GetResource<Texture>(sparklesAnimationId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_BILINEAR_WRAP);

	auto vfxLuxSparklesVS = resourceManager->GetResource<Shader>(vfxLuxSparklesVSId);
	auto vfxLuxSparklesPS = resourceManager->GetResource<Shader>(vfxLuxSparklesPSId);

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE;

	MaterialBuilder materialBuilder{};
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
	Graphics::DirectX12Renderer* renderer, ResourceID perlinNoiseId, ResourceID particleSimulationCSId,
	Graphics::Resources::ResourceID lightParticleBufferId, uint32_t maxParticleNumber)
{
	particleSystemDesc.emitterOrigin = new float3(0.0f, 0.0f, 1.75f);
	particleSystemDesc.emitterRadius = float3(5.0f, 5.0f, 0.5f);
	particleSystemDesc.emitterRadiusOffset = 0.6f;
	particleSystemDesc.minParticleVelocity = float3(-0.5f, -0.5f, -0.3f);
	particleSystemDesc.particleDamping = 0.995f;
	particleSystemDesc.maxParticleVelocity = float3(0.5f, 0.5f, 0.3f);
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
	particleSystemDesc.maxParticlesNumber = maxParticleNumber;
	particleSystemDesc.perlinNoiseId = perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = particleSimulationCSId;
	particleSystemDesc.hasLightSources = true;
	particleSystemDesc.particleLightBufferId = lightParticleBufferId;
	particleSystemDesc.animationTextureId = sparklesAnimationId;
	particleSystemDesc.maxLightIntensity = MAX_LIGHT_INTENSITY;
	particleSystemDesc.lightRange = LIGHT_RANGE;
	particleSystemDesc.forcesNumber = 0u;
	
	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	particleSystem = new SceneEntity::ParticleSystem(commandList, renderer, sparklesMaterial, particleSystemDesc);
}
