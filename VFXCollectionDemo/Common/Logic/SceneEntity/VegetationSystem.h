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

		float3 smallGrassSizeMin;
		float3 mediumGrassSizeMin;
		float3 largeGrassSizeMin;

		float3 smallGrassSizeMax;
		float3 mediumGrassSizeMax;
		float3 largeGrassSizeMax;

		const Terrain* terrain;

		float3* windDirection;
		float* windStrength;

		float2 perlinNoiseTiling;

		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID lightConstantBufferId;
		
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

		struct VegetationBufferDesc
		{
			uint8_t* buffer;
			const Graphics::Resources::TextureDesc& vegetationMapDesc;
			const VegetationSystemDesc& vegetationSystemDesc;
			uint32_t elementNumber;
			uint32_t offset;
			uint32_t quadNumber;
			DirectX::PackedVector::XMUBYTE4 mask;
			float3 grassSizeMin;
			float3 grassSizeMax;
			float tiltAmplitude;
		};

		void FillVegetationBuffer(const VegetationBufferDesc& desc);

		void GetRandomData(const VegetationBufferDesc& desc, floatN& position, floatN& upVector, float2& atlasOffset);

		floatN CalculateRotation(const floatN& upVector, const floatN& upVectorTarget);

		void LoadCache(const std::filesystem::path& fileName, uint8_t* buffer, size_t size);
		void SaveCache(const std::filesystem::path& fileName, const uint8_t* buffer, size_t size);

		struct GrassVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::HALF normalX;
			DirectX::PackedVector::HALF normalY;
			DirectX::PackedVector::HALF normalZ;
			DirectX::PackedVector::HALF normalW;
			DirectX::PackedVector::HALF tangentX;
			DirectX::PackedVector::HALF tangentY;
			DirectX::PackedVector::HALF tangentZ;
			DirectX::PackedVector::HALF tangentW;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		struct Vegetation
		{
		public:
			float4x4 world;

			float2 atlasElementOffset;
			float tiltAmplitude;
			float height;
		};

		struct MutableConstants
		{
		public:
			float4x4 viewProjection;

			float3 cameraPosition;
			float time;

			float2 atlasElementSize;
			float2 perlinNoiseTiling;

			float3 windDirection;
			float windStrength;
		};

		static constexpr uint32_t QUADS_PER_SMALL_GRASS = 1u;
		static constexpr uint32_t QUADS_PER_MEDIUM_GRASS = 3u;
		static constexpr uint32_t QUADS_PER_LARGE_GRASS = 3u;

		static constexpr float SMALL_GRASS_TILT_AMPLITUDE = 0.2f;
		static constexpr float MEDIUM_GRASS_TILT_AMPLITUDE = 0.8f;
		static constexpr float LARGE_GRASS_TILT_AMPLITUDE = 0.6f;

		uint32_t instancesNumber;

		const Camera* _camera;

		MutableConstants* mutableConstantsBuffer;
		float3* windDirection;
		float* windStrength;

		Graphics::Resources::ResourceID lightConstantBufferId;
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
