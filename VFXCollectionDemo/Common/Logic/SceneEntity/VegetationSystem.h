#pragma once

#include "../../../Includes.h"
#include "IDrawable.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"
#include "Terrain.h"

namespace Common::Logic::SceneEntity
{
	struct VegetationSystemTable
	{
	public:
		float3 grassSizeMin;
		float3 grassSizeMax;
		float windInfluence;
	};

	struct VegetationSystemDesc
	{
	public:
		uint32_t atlasRows;
		uint32_t atlasColumns;

		float3 grassSizeMin;
		float3 grassSizeMax;

		bool hasDepthPrepass;
		bool hasDepthPass;
		bool hasDepthCubePass;
		bool hasParticleLighting;
		bool outputVelocity;

		uint32_t lightMatricesNumber;

		const Terrain* terrain;

		float3* windDirection;
		float* windStrength;

		float2 perlinNoiseTiling;

		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID lightConstantBufferId;
		Graphics::Resources::ResourceID lightParticleBufferId;
		Graphics::Resources::ResourceID lightMatricesConstantBufferId;
		
		std::vector<Graphics::Resources::ResourceID> shadowMapIds;
		const std::vector<DxcDefine>* lightDefines;
		
		std::map<uint32_t, VegetationSystemTable> grassTable;

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

		void Update(float time, float deltaTime) override;

		void OnCompute(ID3D12GraphicsCommandList* commandList) override;

		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) override;
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) override;

		void Draw(ID3D12GraphicsCommandList* commandList) override;

		void Release(Graphics::Resources::ResourceManager* resourceManager) override;

	private:
		VegatationSystem() = delete;

		void GenerateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const VegetationSystemDesc& desc);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const VegetationSystemDesc& desc);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const VegetationSystemDesc& desc);

		struct VegetationBufferDesc
		{
			uint8_t* buffer;
			const Graphics::Resources::TextureDesc& vegetationMapDesc;
			const VegetationSystemDesc& vegetationSystemDesc;
			float3 grassSizeMin;
			float3 grassSizeMax;
		};

		struct GrassVertex
		{
		public:
			float3 position;
			uint32_t normalXY;
			uint32_t normalZW;
			uint32_t tangentXY;
			uint32_t tangentZW;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		void FillVegetationBuffer(const VegetationBufferDesc& desc, uint32_t& resultGrassNumber);
		void GetLocationData(const VegetationBufferDesc& desc, uint32_t mapPointIndex, bool isCap,
			floatN& position, floatN& upVector);

		floatN CalculateRotation(const floatN& upVector, const floatN& upVectorTarget);

		GrassVertex SetVertex(const float3& position, uint64_t normal, uint64_t tangent,
			const DirectX::PackedVector::XMHALF2& texCoord);

		void LoadCache(const std::filesystem::path& fileName, std::vector<uint8_t>& buffer);
		void SaveCache(const std::filesystem::path& fileName, const uint8_t* buffer, size_t size);

		struct Vegetation
		{
		public:
			float4x4 world;

			float2 atlasElementOffset;
			float windInfluence;
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

			float zNear;
			float zFar;
			float lastTime;
			float padding;

			float3 lastWindDirection;
			float lastWindStrength;

			float4x4 lastViewProjection;
		};

		static constexpr uint32_t QUADS_PER_GRASS = 3u;

		static constexpr float GRASS_WIND_INFLUENCE = 1.0f;
		static constexpr float GRASS_CAP_WIND_INFLUENCE = 0.1f;
		static constexpr float GRASS_SCATTERING = 0.01f;

		uint32_t instancesNumber;

		const Camera* _camera;

		MutableConstants* mutableConstantsBuffer;
		float3* windDirection;
		float* windStrength;

		Graphics::Resources::ResourceID lightConstantBufferId;
		Graphics::Resources::ResourceID lightMatricesConstantBufferId;
		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID vegetationBufferId;

		Graphics::Resources::ResourceID albedoMapId;
		Graphics::Resources::ResourceID normalMapId;

		Graphics::Resources::ResourceID vegetationVSId;
		Graphics::Resources::ResourceID vegetationPSId;

		Graphics::Resources::ResourceID vegetationDepthPassVSId;
		Graphics::Resources::ResourceID vegetationDepthPassPSId;
		Graphics::Resources::ResourceID vegetationDepthCubePassVSId;
		Graphics::Resources::ResourceID vegetationDepthCubePassGSId;
		Graphics::Resources::ResourceID vegetationDepthCubePassPSId;
		
		Graphics::Assets::Mesh* _mesh;
		Graphics::Assets::Material* _material;
		Graphics::Assets::Material* materialDepthPrepass;
		Graphics::Assets::Material* materialDepthPass;
		Graphics::Assets::Material* materialDepthCubePass;

		std::mt19937 randomEngine;
	};
}
