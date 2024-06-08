#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"
#include "Terrain.h"

namespace Common::Logic::SceneEntity
{
	struct VegetationSystemDesc
	{
	public:
		uint32_t smallGrassNumber;
		uint32_t mediumGrassNumber;
		uint32_t largeGrassNumber;

		uint32_t atlasRows;
		uint32_t atlasColumns;

		uint32_t smallGrassStartAtlasIndex;
		uint32_t smallGrassEndAtlasIndex;
		uint32_t mediumGrassStartAtlasIndex;
		uint32_t mediumGrassEndAtlasIndex;
		uint32_t largeGrassStartAtlasIndex;
		uint32_t largeGrassEndAtlasIndex;

		const Terrain* terrain;

		std::mt19937 randomEngine;
		float2 perlinNoiseSize;

		Graphics::Resources::ResourceID perlinNoiseId;
		
		std::filesystem::path vegetationCacheFileName;
		std::filesystem::path vegetationMapFileName;
		std::filesystem::path albedoMapFileName;
		std::filesystem::path normalMapFileName;
	};

	class VegatationSystem : public IDrawable
	{
	public:
		VegatationSystem(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			const VegetationSystemDesc& desc, const Camera* camera);
		~VegatationSystem() override;

		void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;
		void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VegatationSystem() = delete;

		void GenerateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void CreateMaterial(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::Resources::ResourceID perlinNoiseId);

		void FillVegetationBuffer(uint8_t* buffer, const Graphics::Resources::TextureDesc& vegetationMapDesc,
			const VegetationSystemDesc& desc, uint32_t elementNumber, uint32_t offset, uint32_t quadNumber,
			DirectX::PackedVector::XMUBYTE4 mask);

		void GetRandomData(const Graphics::Resources::TextureDesc& vegetationMapDesc, const VegetationSystemDesc& desc,
			DirectX::PackedVector::XMUBYTE4 mask, floatN& position, floatN& upVector, float2& atlasOffset);

		floatN CalculateRotation(const floatN& upVector, const floatN& upVectorTarget);

		void LoadCache(const std::filesystem::path& fileName, uint8_t* buffer, size_t size);
		void SaveCache(const std::filesystem::path& fileName, const uint8_t* buffer, size_t size);

		struct GrassVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		struct Vegetation
		{
		public:
			float4x4 world;

			float2 atlasElementOffset;
			float2 padding;
		};

		struct MutableConstants
		{
		public:
			float4x4 viewProjection;

			float3 cameraDirection;
			float time;

			float2 atlasElementSize;
			float2 padding;
		};

		static constexpr uint32_t QUADS_PER_SMALL_GRASS = 1u;
		static constexpr uint32_t QUADS_PER_MEDIUM_GRASS = 3u;
		static constexpr uint32_t QUADS_PER_LARGE_GRASS = 3u;
		static constexpr float2 SMALL_GRASS_SIZE_MIN = float2(0.01f, 0.01f);
		static constexpr float2 SMALL_GRASS_SIZE_MAX = float2(0.015f, 0.015f);
		static constexpr float2 MEDIUM_GRASS_SIZE_MIN = float2(0.03f, 0.03f);
		static constexpr float2 MEDIUM_GRASS_SIZE_MAX = float2(0.035f, 0.04f);
		static constexpr float2 LARGE_GRASS_SIZE_MIN = float2(0.03f, 0.045f);
		static constexpr float2 LARGE_GRASS_SIZE_MAX = float2(0.04f, 0.08f);

		uint32_t instancesNumber;

		const Camera* _camera;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID vegetationBufferId;

		Graphics::Resources::ResourceID albedoMapId;
		Graphics::Resources::ResourceID normalMapId;

		Graphics::Resources::ResourceID vegetationVSId;
		Graphics::Resources::ResourceID vegetationPSId;

		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;

		std::mt19937 randomEngine;
	};
}
