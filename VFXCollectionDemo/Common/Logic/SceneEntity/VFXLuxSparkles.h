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
			Graphics::Resources::ResourceID particleSimulationCSId, Camera* camera);
		~VFXLuxSparkles() override;

		void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;
		void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;

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
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID particleSimulationCSId);

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
