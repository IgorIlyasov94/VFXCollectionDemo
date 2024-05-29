#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::SceneEntity
{
	class VFXLux : public IDrawable
	{
	public:
		VFXLux(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceID vfxAtlasId);
		~VFXLux() override;

		void Draw(ID3D12GraphicsCommandList* commandList) override;

	private:
		VFXLux() = delete;

		void CreateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMaterials(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		uint32_t SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

		struct VFXVertex
		{
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
			uint32_t color;
		};

		static constexpr uint32_t TOTAL_SEGMENT_NUMBER = 24;
		static constexpr uint32_t VERTICES_PER_ROW = 4u;
		static constexpr uint32_t VERTICES_NUMBER = TOTAL_SEGMENT_NUMBER * 8u + VERTICES_PER_ROW;

		static constexpr uint32_t INDICES_PER_TRIANGLE = 6u;
		static constexpr uint32_t INDICES_PER_SEGMENT = INDICES_PER_TRIANGLE * 3u;
		static constexpr uint32_t INDICES_NUMBER = TOTAL_SEGMENT_NUMBER * INDICES_PER_SEGMENT;

		static constexpr float GEOMETRY_WIDTH = 0.5f;
		static constexpr float GEOMETRY_HEIGHT = 5.0f;
		static constexpr float GEOMETRY_FADING_WIDTH = 0.125f;

		static constexpr uint32_t INDEX_OFFSETS[INDICES_PER_TRIANGLE] =
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

		Graphics::Assets::Mesh* vfxMesh;
	};
}
