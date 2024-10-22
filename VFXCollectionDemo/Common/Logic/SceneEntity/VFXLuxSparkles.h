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
	class VFXLuxSparkles : public IDrawable
	{
	public:
		VFXLuxSparkles(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID vfxAtlasId,
			Graphics::Resources::ResourceID lightParticleBufferId, Graphics::Resources::ResourceID particleSimulationCSId,
			uint32_t maxParticleNumber, Camera* camera);
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
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID vfxAtlasId);

		void CreateParticleSystems(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID particleSimulationCSId,
			Graphics::Resources::ResourceID lightParticleBufferId, uint32_t maxParticleNumber);

		static constexpr float MAX_LIGHT_INTENSITY = 12.0f;
		static constexpr float LIGHT_RANGE = 1.5f;

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

		VFXSparklesConstants* sparklesConstants;

		ParticleSystemDesc particleSystemDesc;

		Camera* _camera;

		Graphics::Resources::ResourceID sparklesConstantsId;
		Graphics::Resources::ResourceID sparklesAnimationId;

		Graphics::Resources::ResourceID vfxLuxSparklesVSId;
		Graphics::Resources::ResourceID vfxLuxSparklesPSId;
		
		Graphics::Assets::Material* sparklesMaterial;

		SceneEntity::IDrawable* particleSystem;
	};
}
