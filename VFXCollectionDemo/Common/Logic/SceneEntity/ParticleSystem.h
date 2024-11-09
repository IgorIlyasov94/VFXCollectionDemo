#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/ComputeObject.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::SceneEntity
{
	enum class ParticleSystemForceType : uint32_t
	{
		ATTRACTOR = 0u,
		CIRCULAR = 1u
	};

	struct ParticleSystemForce
	{
	public:
		float3 position;
		float strength;
		float3 axis;
		ParticleSystemForceType type;
		float nAccelerationCoeff;
		float tAccelerationCoeff;
		float2 padding;
	};

	struct ParticleSystemDesc
	{
	public:
		float3* emitterOrigin;

		float3 emitterRadius;
		float lightRange;

		float3 emitterRadiusOffset;
		float maxLightIntensity;

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
		bool hasLightSources;
		
		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID animationTextureId;
		Graphics::Resources::ResourceID particleLightBufferId;
		Graphics::Resources::ResourceID particleSimulationCSId;

		ParticleSystemForce* forces;
		uint32_t forcesNumber;

		float2 perlinNoiseSize;
	};

	struct Particle
	{
		float3 position;
		float rotation;

		float3 velocity;
		float rotationSpeed;

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

		void Update(float time, float deltaTime) override;

		void OnCompute(ID3D12GraphicsCommandList* commandList) override;

		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) override;
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void Draw(ID3D12GraphicsCommandList* commandList) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

		static constexpr uint32_t MAX_FORCES_NUMBER = 4u;

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
			float padding;

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

			float maxLightIntensity;
			float3 emitterRadius;

			float lightRange;
			float3 emitterRadiusOffset;

			ParticleSystemForce forces[MAX_FORCES_NUMBER];
		};

		static constexpr uint32_t THREADS_PER_GROUP = 64u;

		MutableConstants* mutableConstantsBuffer;

		ParticleSystemDesc _desc;

		Graphics::Resources::GPUResource* particleBufferGPUResource;
		Graphics::Resources::GPUResource* particleLightBufferGPUResource;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID particleBufferId;

		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;
		Graphics::Assets::ComputeObject* particleSimulation;

		std::mt19937 randomEngine;
	};
}
