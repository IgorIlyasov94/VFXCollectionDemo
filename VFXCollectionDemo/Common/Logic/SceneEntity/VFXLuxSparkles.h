#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "Camera.h"
#include "ParticleSystem.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::SceneEntity
{
	struct VFXLuxSparklesDesc
	{
		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID vfxAtlasId;
		Graphics::Resources::ResourceID lightParticleBufferId;
		Graphics::Resources::ResourceID firefliesSimulationCSId;
		Graphics::Resources::ResourceID sparkleSimulationCSId;
		uint32_t firefliesNumber;
		uint32_t sparklesNumber;
	};

	class VFXLuxSparkles : public IDrawable
	{
	public:
		VFXLuxSparkles(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Camera* camera, const VFXLuxSparklesDesc& desc);
		~VFXLuxSparkles() override;

		void Update(float time, float deltaTime) override;

		void OnCompute(ID3D12GraphicsCommandList* commandList) override;

		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) override;
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void Draw(ID3D12GraphicsCommandList* commandList) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VFXLuxSparkles() = delete;

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const VFXLuxSparklesDesc& desc);

		void CreateParticleSystems(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			const VFXLuxSparklesDesc& desc);

		static constexpr uint32_t FIREFLY_SPRITE_INDEX = 10u;
		static constexpr uint32_t SPARKLE_SPRITE_INDEX = 10u;
		static constexpr uint32_t VFX_ATLAS_SIZE = 8u;

		static constexpr float MAX_LIGHT_INTENSITY = 20.0f;
		static constexpr float LIGHT_RANGE = 3.5f;

		struct VFXSparklesConstants
		{
			float4x4 invView;
			float4x4 viewProjection;

			float2 atlasElementOffset;
			float2 atlasElementSize;

			float colorIntensity;
			float3 perlinNoiseTiling;

			float3 perlinNoiseScrolling;
			float particleTurbulence;

			float time;
			float3 padding;
		};

		VFXSparklesConstants* firefliesConstants;
		VFXSparklesConstants* sparklesConstants;

		std::vector<ParticleSystemForce> firefliesForces;
		std::vector<ParticleSystemForce> sparklesForces;
		float3 firefliesOrigin;
		float3 sparklesOrigin;

		Camera* _camera;

		Graphics::Resources::ResourceID firefliesConstantsId;
		Graphics::Resources::ResourceID sparklesConstantsId;
		Graphics::Resources::ResourceID sparklesAnimationId;

		Graphics::Resources::ResourceID vfxLuxSparklesVSId;
		Graphics::Resources::ResourceID vfxLuxSparklesPSId;
		
		Graphics::Assets::Material* firefliesMaterial;
		Graphics::Assets::Material* sparklesMaterial;

		SceneEntity::IDrawable* firefliesParticleSystem;
		SceneEntity::IDrawable* sparklesParticleSystem;
	};
}
