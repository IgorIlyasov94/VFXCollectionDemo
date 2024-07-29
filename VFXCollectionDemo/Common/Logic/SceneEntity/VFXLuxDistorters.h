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
	class VFXLuxDistorters : public IDrawable
	{
	public:
		VFXLuxDistorters(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID vfxAtlasId,
			Graphics::Resources::ResourceID particleSimulationCSId, Camera* camera);
		~VFXLuxDistorters() override;

		void Update(float time, float deltaTime) override;

		void OnCompute(ID3D12GraphicsCommandList* commandList) override;

		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) override;
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void Draw(ID3D12GraphicsCommandList* commandList) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VFXLuxDistorters() = delete;

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::Resources::ResourceID vfxAtlasId, Graphics::Resources::ResourceID perlinNoiseId);

		void CreateParticleSystems(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID perlinNoiseId, Graphics::Resources::ResourceID particleSimulationCSId);

		struct VFXDistortersConstants
		{
			float4x4 invView;
			float4x4 viewProjection;

			float2 atlasElementOffset;
			float2 atlasElementSize;

			float2 noiseTiling;
			float2 noiseScrollSpeed;

			float time;
			float deltaTime;
			float particleNumber;
			float noiseStrength;

			float velocityStrength;
			float3 padding;
		};

		VFXDistortersConstants* distortersConstants;

		ParticleSystemDesc particleSystemDesc;

		Camera* _camera;

		Graphics::Resources::ResourceID distortersConstantsId;
		Graphics::Resources::ResourceID distortersAnimationId;

		Graphics::Resources::ResourceID vfxLuxDistorterVSId;
		Graphics::Resources::ResourceID vfxLuxDistorterPSId;

		Graphics::Assets::Material* distortersMaterial;

		SceneEntity::IDrawable* particleSystem;
	};
}
