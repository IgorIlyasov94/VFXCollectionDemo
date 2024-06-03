#include "ParticleSystem.h"
#include "../../../Graphics/Assets/ComputeObjectBuilder.h"
#include "../../../Graphics/Assets/Loaders/HLSLLoader.h"
#include "../../Utilities.h"

using namespace Graphics;
using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace DirectX::PackedVector;
using namespace Graphics::Assets::Loaders;

Common::Logic::SceneEntity::ParticleSystem::ParticleSystem(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, Graphics::Assets::Material* material, const ParticleSystemDesc& desc)
	: _material(material), _desc(desc)
{
	std::random_device randomDevice;
	randomEngine = std::mt19937(randomDevice());

	_desc.attractorsNumber = std::clamp(_desc.attractorsNumber, 0u, MAX_ATTRACTORS_NUMBER);

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	CreateConstantBuffers(device, commandList, resourceManager);
	CreateBuffers(device, commandList, resourceManager);
	CreateMesh(device, commandList, resourceManager);
	CreateComputeObject(device, resourceManager);

	auto particleBufferResource = resourceManager->GetResource<RWBuffer>(particleBufferId);

	_material->UpdateBuffer(0u, particleBufferResource->resourceGPUAddress);
}

Common::Logic::SceneEntity::ParticleSystem::~ParticleSystem()
{

}

void Common::Logic::SceneEntity::ParticleSystem::OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	mutableConstantsBuffer->emitterOrigin = *_desc.emitterOrigin;
	mutableConstantsBuffer->time = time;
	mutableConstantsBuffer->deltaTime = deltaTime;

	mutableConstantsBuffer->random0 = Utilities::Random4(randomEngine);
	mutableConstantsBuffer->random1 = Utilities::Random4(randomEngine);

	for (uint32_t attractorIndex = 0u; attractorIndex < _desc.attractorsNumber; attractorIndex++)
		mutableConstantsBuffer->attractors[attractorIndex] = _desc.attractors[attractorIndex];

	auto numGroups = static_cast<uint32_t>(std::lroundf(std::ceilf(_desc.maxParticlesNumber / static_cast<float>(THREADS_PER_GROUP))));

	particleBufferGPUResource->EndBarrier(commandList);

	particleSimulation->Set(commandList);
	particleSimulation->Dispatch(commandList, numGroups, 1u, 1u);

	particleBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void Common::Logic::SceneEntity::ParticleSystem::Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	particleBufferGPUResource->UAVBarrier(commandList);
	particleBufferGPUResource->EndBarrier(commandList);
	
	_material->Set(commandList);
	_mesh->Draw(commandList, _desc.maxParticlesNumber);

	particleBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void Common::Logic::SceneEntity::ParticleSystem::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	_mesh->Release(resourceManager);
	delete _mesh;

	delete particleSimulation;
}

void Common::Logic::SceneEntity::ParticleSystem::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(MutableConstants));
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	mutableConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	mutableConstantsBuffer = reinterpret_cast<MutableConstants*>(mutableConstantsResource->resourceCPUAddress);
	mutableConstantsBuffer->emitterOrigin = *_desc.emitterOrigin;
	mutableConstantsBuffer->emitterRadius = _desc.emitterRadius;
	mutableConstantsBuffer->minParticleVelocity = _desc.minParticleVelocity;
	mutableConstantsBuffer->particleDamping = _desc.particleDamping;
	mutableConstantsBuffer->maxParticleVelocity = _desc.maxParticleVelocity;
	mutableConstantsBuffer->particleTurbulence = std::clamp(_desc.particleTurbulence, 0.0f, 1.0f);
	mutableConstantsBuffer->minRotation = _desc.minRotation;
	mutableConstantsBuffer->maxRotation = _desc.maxRotation;
	mutableConstantsBuffer->minRotationSpeed = _desc.minRotationSpeed;
	mutableConstantsBuffer->maxRotationSpeed = _desc.maxRotationSpeed;
	mutableConstantsBuffer->minSize = _desc.minSize;
	mutableConstantsBuffer->maxSize = _desc.maxSize;
	mutableConstantsBuffer->minLifeSec = _desc.minLifeSec;
	mutableConstantsBuffer->maxLifeSec = _desc.maxLifeSec;
	mutableConstantsBuffer->time = 0.0f;
	mutableConstantsBuffer->deltaTime = 0.0f;
	mutableConstantsBuffer->averageParticleEmit = _desc.averageParticleEmitPerSecond;
	mutableConstantsBuffer->maxParticlesNumber = _desc.maxParticlesNumber;
	mutableConstantsBuffer->perlinNoiseSize = _desc.perlinNoiseSize;

	mutableConstantsBuffer->random0 = Utilities::Random4(randomEngine);
	mutableConstantsBuffer->random1 = Utilities::Random4(randomEngine);

	for (uint32_t attractorIndex = 0u; attractorIndex < MAX_ATTRACTORS_NUMBER; attractorIndex++)
		if (attractorIndex < _desc.attractorsNumber)
			mutableConstantsBuffer->attractors[attractorIndex] = _desc.attractors[attractorIndex];
		else
			mutableConstantsBuffer->attractors[attractorIndex] = {};
}

void Common::Logic::SceneEntity::ParticleSystem::CreateBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	auto particles = new Particle[_desc.maxParticlesNumber];
	std::fill(&particles[0], &particles[0] + _desc.maxParticlesNumber, Particle{});

	BufferDesc particleBufferDesc(particles, sizeof(Particle) * _desc.maxParticlesNumber);
	
	particleBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::RW_BUFFER, particleBufferDesc);

	auto particleBufferResource = resourceManager->GetResource<RWBuffer>(particleBufferId);
	particleBufferGPUResource = particleBufferResource->resource;

	delete[] particles;
}

void Common::Logic::SceneEntity::ParticleSystem::CreateMesh(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	MeshDesc meshDesc{};
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::TEXCOORD0;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	meshDesc.verticesNumber = 4u;
	meshDesc.indicesNumber = 6u;

	auto halfZero = XMConvertFloatToHalf(0.0f);
	auto halfOne = XMConvertFloatToHalf(1.0f);

	ParticleVertex vertices[4u]
	{
		{ float3(-1.0f, 1.0f, 0.0f), XMHALF2(halfZero, halfZero) },
		{ float3(1.0f, 1.0f, 0.0f), XMHALF2(halfOne, halfZero) },
		{ float3(-1.0f, -1.0f, 0.0f), XMHALF2(halfZero, halfOne) },
		{ float3(1.0f, -1.0f, 0.0f), XMHALF2(halfOne, halfOne) },
	};

	uint16_t indices[6u]
	{
		0u, 1u, 2u,
		2u, 1u, 3u
	};

	BufferDesc vertexBufferDesc(&vertices[0], sizeof(vertices));
	BufferDesc indexBufferDesc(&indices[0], sizeof(indices));

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::VERTEX_BUFFER, vertexBufferDesc);
	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::INDEX_BUFFER, indexBufferDesc);

	_mesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::ParticleSystem::CreateComputeObject(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	auto mutableConstantsResource = resourceManager->GetResource<ConstantBuffer>(mutableConstantsId);
	auto particleBufferResource = resourceManager->GetResource<RWBuffer>(particleBufferId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(_desc.perlinNoiseId);

	auto particleSimulationResource = resourceManager->GetResource<Shader>(_desc.particleSimulationCSId);

	ComputeObjectBuilder computeObjectBuilder{};
	computeObjectBuilder.SetConstantBuffer(0u, mutableConstantsResource->resourceGPUAddress);
	computeObjectBuilder.SetRWBuffer(0u, particleBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetTexture(0u, perlinNoiseResource->srvDescriptor.gpuDescriptor);
	computeObjectBuilder.SetShader(particleSimulationResource->bytecode);

	particleSimulation = computeObjectBuilder.Compose(device);
}
