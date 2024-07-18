#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "Camera.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::SceneEntity
{
	class VFXLux : public IDrawable
	{
	public:
		VFXLux(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID perlinNoiseId, Camera* camera, const float3& position);
		~VFXLux() override;

		void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;
		void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VFXLux() = delete;

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const float3& position);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateCircleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateHaloMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::Resources::ResourceID perlinNoiseId);

		uint8_t FloatToColorChannel(float value);
		uint32_t SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

		struct VFXVertex
		{
			float3 position;
			uint32_t color;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		struct VFXPillarConstants
		{
			float4x4 invView;
			float4x4 viewProjection;
			float4 color0;
			float4 color1;
			float3 worldPosition;
			float time;
			float2 tiling0;
			float2 tiling1;
			float2 scrollSpeed0;
			float2 scrollSpeed1;
			float colorIntensity;
			float alphaSharpness;
			float distortionStrength;
			float padding;
		};

		struct VFXHaloConstants
		{
			float4x4 invView;
			float4x4 viewProjection;
			float3 worldPosition;
			float time;
			float2 tiling0;
			float2 tiling1;
			float2 scrollSpeed0;
			float2 scrollSpeed1;
			float colorIntensity;
			float alphaSharpness;
			float distortionStrength;
			float padding;
		};

		static constexpr uint32_t TOTAL_SEGMENT_NUMBER = 12u;
		static constexpr uint32_t TOTAL_RING_NUMBER = 4u;
		static constexpr uint32_t TOTAL_RIBBON_NUMBER = TOTAL_RING_NUMBER - 1;
		static constexpr uint32_t VERTICES_PER_RING = TOTAL_SEGMENT_NUMBER + 1u;
		static constexpr uint32_t VERTICES_NUMBER = TOTAL_RING_NUMBER * VERTICES_PER_RING;

		static constexpr uint32_t HALO_SECTOR_NUMBER = 2u;
		static constexpr uint32_t HALO_SEGMENT_NUMBER = 8u;
		static constexpr uint32_t HALO_SEGMENTS_PER_SECTOR = HALO_SEGMENT_NUMBER / HALO_SECTOR_NUMBER;
		static constexpr uint32_t HALO_VERTICES_PER_RING = HALO_SEGMENT_NUMBER + 2u;
		static constexpr uint32_t HALO_VERTICES_PER_SECTOR = HALO_VERTICES_PER_RING / HALO_SECTOR_NUMBER;
		static constexpr uint32_t HALO_VERTICES_NUMBER = TOTAL_RING_NUMBER * HALO_VERTICES_PER_RING;

		static constexpr uint32_t INDICES_PER_SEGMENT = 6u;
		static constexpr uint32_t INDICES_PER_RIBBON = TOTAL_SEGMENT_NUMBER * INDICES_PER_SEGMENT;
		static constexpr uint32_t INDICES_NUMBER = TOTAL_RIBBON_NUMBER * INDICES_PER_RIBBON;

		static constexpr uint32_t HALO_INDICES_PER_RIBBON = (TOTAL_SEGMENT_NUMBER - 4) * INDICES_PER_SEGMENT;
		static constexpr uint32_t HALO_INDICES_NUMBER = TOTAL_RIBBON_NUMBER * HALO_INDICES_PER_RIBBON;

		static constexpr float RING_START_OFFSET = 0.4f;
		static constexpr float RING_WIDTH = 0.5f;
		static constexpr float RING_FADING_WIDTH = 0.3f;
		static constexpr float CIRCLE_WIDTH = RING_WIDTH + RING_FADING_WIDTH * 2.0f;

		static constexpr float HALO_RING_START_OFFSET = 0.5f;
		static constexpr float HALO_RING_WIDTH = 1.4f;
		static constexpr float HALO_RING_FADING_WIDTH = 0.7f;
		static constexpr float HALO_WIDTH = HALO_RING_WIDTH + HALO_RING_FADING_WIDTH * 2.0f;

		static constexpr uint32_t INDEX_OFFSETS[INDICES_PER_SEGMENT] =
		{
			VERTICES_PER_RING,
			VERTICES_PER_RING + 1u,
			0u,
			0u,
			VERTICES_PER_RING + 1u,
			1u
		};

		static constexpr uint32_t HALO_INDEX_OFFSETS[INDICES_PER_SEGMENT] =
		{
			HALO_VERTICES_PER_RING,
			HALO_VERTICES_PER_RING + 1u,
			0u,
			0u,
			HALO_VERTICES_PER_RING + 1u,
			1u
		};

		static constexpr float RING_RADIUSES[TOTAL_RING_NUMBER] =
		{
			RING_START_OFFSET,
			RING_START_OFFSET + RING_FADING_WIDTH,
			RING_START_OFFSET + RING_FADING_WIDTH + RING_WIDTH,
			RING_START_OFFSET + CIRCLE_WIDTH
		};

		static constexpr float HALO_RING_RADIUSES[TOTAL_RING_NUMBER] =
		{
			HALO_RING_START_OFFSET,
			HALO_RING_START_OFFSET + HALO_RING_FADING_WIDTH,
			HALO_RING_START_OFFSET + HALO_RING_FADING_WIDTH + HALO_RING_WIDTH,
			HALO_RING_START_OFFSET + HALO_WIDTH
		};

		float4x4 circleWorld;
		float colorIntensity;
		float haloColorIntensity;
		float animationSpeed;
		
		VFXPillarConstants* circleConstants;
		VFXHaloConstants* haloConstants;

		Camera* _camera;
		
		Graphics::Resources::ResourceID circleConstantsId;
		Graphics::Resources::ResourceID haloConstantsId;

		Graphics::Resources::ResourceID vfxLuxHaloSpectrumId;

		Graphics::Resources::ResourceID vfxLuxCircleVSId;
		Graphics::Resources::ResourceID vfxLuxHaloVSId;
		Graphics::Resources::ResourceID vfxLuxCirclePSId;
		Graphics::Resources::ResourceID vfxLuxHaloPSId;
		
		Graphics::Assets::Mesh* circleMesh;
		Graphics::Assets::Mesh* haloMesh;
		Graphics::Assets::Material* circleMaterial;
		Graphics::Assets::Material* haloMaterial;
	};
}
