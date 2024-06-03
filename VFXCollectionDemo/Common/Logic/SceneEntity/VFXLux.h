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
			Graphics::Resources::ResourceID perlinNoiseId, Camera* camera);
		~VFXLux() override;

		void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;
		void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VFXLux() = delete;

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void CreateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
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
			float4x4 world;
			float4x4 viewProjection;
			float2 tiling0;
			float2 tiling1;
			float2 scrollSpeed0;
			float2 scrollSpeed1;
			float4 displacementStrength;
			float4 color0;
			float4 color1;
			float alphaIntensity;
			float colorIntensity;
			float time;
			float padding;
		};

		static constexpr uint32_t TOTAL_SEGMENT_NUMBER = 24;
		static constexpr uint32_t VERTICES_PER_ROW = 4u;
		static constexpr uint32_t VERTICES_NUMBER = TOTAL_SEGMENT_NUMBER * 4u + VERTICES_PER_ROW;

		static constexpr uint32_t INDICES_PER_QUAD = 6u;
		static constexpr uint32_t INDICES_PER_SEGMENT = INDICES_PER_QUAD * 3u;
		static constexpr uint32_t INDICES_NUMBER = TOTAL_SEGMENT_NUMBER * INDICES_PER_SEGMENT;

		static constexpr float GEOMETRY_WIDTH = 0.3f;
		static constexpr float GEOMETRY_HEIGHT = 5.0f;
		static constexpr float GEOMETRY_FADING_WIDTH = 0.125f;

		static constexpr uint32_t INDEX_OFFSETS[INDICES_PER_QUAD] =
		{
			0u, 1u, 4u, 4u, 1u, 5u
		};

		static constexpr float GEOMETRY_X_OFFSETS[VERTICES_PER_ROW] = 
		{
			-GEOMETRY_WIDTH / 2.0f,
			-GEOMETRY_WIDTH / 2.0f + GEOMETRY_FADING_WIDTH,
			GEOMETRY_WIDTH / 2.0f - GEOMETRY_FADING_WIDTH,
			GEOMETRY_WIDTH / 2.0f
		};

		float4x4 pillarWorld;

		VFXPillarConstants* pillarConstants;

		Camera* _camera;

		Graphics::Resources::ResourceID pillarConstantsId;
		
		Graphics::Resources::ResourceID vfxLuxPillarVSId;
		Graphics::Resources::ResourceID vfxLuxPillarPSId;

		Graphics::Assets::Mesh* pillarMesh;
		Graphics::Assets::Material* pillarMaterial;
	};
}
