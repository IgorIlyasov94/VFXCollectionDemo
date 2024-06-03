#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/ComputeObject.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::SceneEntity
{
	struct ParticleSystemAttractor
	{
	public:
		float3 position;
		float strength;
	};

	struct ParticleSystemDesc
	{
	public:
		float3* emitterOrigin;
		float emitterRadius;

		float3 minParticleVelocity;
		float particleDamping;

		float3 maxParticleVelocity;
		float particleTurbulence;

		float minRotation;
		float maxRotation;
		float minRotationSpeed;
		float maxRotationSpeed;

		float2 minSize;
		float2 maxSize;

		float minLifeSec;
		float maxLifeSec;

		uint32_t averageParticleEmitPerSecond;
		uint32_t maxParticlesNumber;
		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID particleSimulationCSId;

		ParticleSystemAttractor* attractors;
		uint32_t attractorsNumber;

		float2 perlinNoiseSize;
	};

	struct Particle
	{
		float3 position;
		float rotation;

		float3 previousPosition;
		float previousRotation;

		float2 size;
		float life;
		float startLife;
	};

	class ParticleSystem : public IDrawable
	{
	public:
		ParticleSystem(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Assets::Material* material, const ParticleSystemDesc& desc);
		~ParticleSystem() override;

		void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;
		void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

		static constexpr uint32_t MAX_ATTRACTORS_NUMBER = 8u;

	private:
		ParticleSystem() = delete;

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateComputeObject(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		struct ParticleVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		struct MutableConstants
		{
		public:
			float3 emitterOrigin;
			float emitterRadius;

			float3 minParticleVelocity;
			float particleDamping;

			float3 maxParticleVelocity;
			float particleTurbulence;

			float minRotation;
			float maxRotation;
			float minRotationSpeed;
			float maxRotationSpeed;

			float2 minSize;
			float2 maxSize;

			float minLifeSec;
			float maxLifeSec;
			float time;
			float deltaTime;
			
			uint32_t averageParticleEmit;
			uint32_t maxParticlesNumber;
			float2 perlinNoiseSize;

			float4 random0;
			float4 random1;

			ParticleSystemAttractor attractors[MAX_ATTRACTORS_NUMBER];
		};

		static constexpr uint32_t THREADS_PER_GROUP = 64u;

		MutableConstants* mutableConstantsBuffer;

		ParticleSystemDesc _desc;

		Graphics::Resources::GPUResource* particleBufferGPUResource;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID particleBufferId;

		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;
		Graphics::Assets::ComputeObject* particleSimulation;

		std::mt19937 randomEngine;
	};
}
