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
	: particleSystemDesc{}
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

void Common::Logic::SceneEntity::VFXLuxDistorters::Update(float time, float deltaTime)
{
	distortersConstants->invView = _camera->GetInvView();
	distortersConstants->viewProjection = _camera->GetViewProjection();
	distortersConstants->time = time;
	distortersConstants->deltaTime = deltaTime;

	particleSystem->Update(time, deltaTime);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::OnCompute(ID3D12GraphicsCommandList* commandList)
{
	particleSystem->OnCompute(commandList);
}

void Common::Logic::SceneEntity::VFXLuxDistorters::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::VFXLuxDistorters::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLuxDistorters::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLuxDistorters::Draw(ID3D12GraphicsCommandList* commandList)
{
	particleSystem->Draw(commandList);
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
	delete[] particleSystemDesc.forces;
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
	distortersConstants->atlasElementOffset = float2(0.0f, 0.0f);
	distortersConstants->atlasElementSize = float2(1.0f / 8.0f, 1.0f / 8.0f);
	distortersConstants->noiseTiling = float2(1.7f, 1.6f);
	distortersConstants->noiseScrollSpeed = float2(0.12f, 0.3f);
	distortersConstants->time = 0.0f;
	distortersConstants->deltaTime = 0.0f;
	distortersConstants->particleNumber = 10.0f;
	distortersConstants->noiseStrength = 0.002f;
	distortersConstants->velocityStrength = 0.02f;
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
	particleSystemDesc.emitterOrigin = new float3(0.0f, 0.0f, 1.35f);
	particleSystemDesc.emitterRadiusOffset = 0.3f;
	particleSystemDesc.emitterRadius = float3(0.3f, 0.3f, 0.3f);
	particleSystemDesc.minParticleVelocity = float3(-0.2f, -0.4f, -0.05f);
	particleSystemDesc.particleDamping = 0.996f;
	particleSystemDesc.maxParticleVelocity = float3(0.2f, 0.2f, 0.15f);
	particleSystemDesc.particleTurbulence = 0.0f;
	particleSystemDesc.minRotation = 0.0f;
	particleSystemDesc.maxRotation = static_cast<float>(std::numbers::pi * 2.0);
	particleSystemDesc.minRotationSpeed = 0.0f;
	particleSystemDesc.maxRotationSpeed = 0.0f;
	particleSystemDesc.minSize = float2(1.3f, 1.3f);
	particleSystemDesc.maxSize = float2(3.2f, 3.2f);
	particleSystemDesc.minLifeSec = 2.4f;
	particleSystemDesc.maxLifeSec = 4.0f;
	particleSystemDesc.averageParticleEmitPerSecond = 25u;
	particleSystemDesc.maxParticlesNumber = 5u;
	particleSystemDesc.perlinNoiseId = perlinNoiseId;
	particleSystemDesc.particleSimulationCSId = particleSimulationCSId;
	particleSystemDesc.forcesNumber = 0u;
	
	particleSystemDesc.perlinNoiseSize = float2(1024.0f, 1024.0f);

	particleSystem = new SceneEntity::ParticleSystem(commandList, renderer, distortersMaterial, particleSystemDesc);
}
