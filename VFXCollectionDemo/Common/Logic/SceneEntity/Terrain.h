#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/DirectX12Renderer.h"
#include "LightingSystem.h"
#include "Camera.h"

namespace Common::Logic::SceneEntity
{
	struct TerrainDesc
	{
		float3 origin;
		uint32_t verticesPerWidth;
		uint32_t verticesPerHeight;
		float3 size;

		float2 map0Tiling;
		float2 map1Tiling;
		float2 map2Tiling;
		float2 map3Tiling;

		bool hasDepthPrepass;
		bool hasParticleLighting;
		bool outputVelocity;

		Graphics::Resources::ResourceID lightConstantBufferId;
		Graphics::Resources::ResourceID lightParticleBufferId;
		std::vector<Graphics::Resources::ResourceID> shadowMapIds;
		const std::vector<DxcDefine>* shaderDefines;
		
		Graphics::Assets::Material* materialDepthPass;
		Graphics::Assets::Material* materialDepthCubePass;

		std::filesystem::path terrainFileName;
		std::filesystem::path heightMapFileName;
		std::filesystem::path blendMapFileName;
		std::filesystem::path map0AlbedoFileName;
		std::filesystem::path map1AlbedoFileName;
		std::filesystem::path map2AlbedoFileName;
		std::filesystem::path map3AlbedoFileName;
		std::filesystem::path map0NormalFileName;
		std::filesystem::path map1NormalFileName;
		std::filesystem::path map2NormalFileName;
		std::filesystem::path map3NormalFileName;
	};

	class Terrain
	{
	public:
		Terrain(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			const TerrainDesc& desc);
		~Terrain();

		const float4x4& GetWorld() const;
		float GetHeight(const float2& position) const;
		float3 GetNormal(const float2& position) const;

		const float3& GetSize() const noexcept;
		const float3& GetMinCorner() const noexcept;

		void Update(const Camera* camera, float time);
		void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList);
		void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex);
		void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex);
		void Draw(ID3D12GraphicsCommandList* commandList);

		void Release(Graphics::Resources::ResourceManager* resourceManager);

	private:
		Terrain() = delete;

		void LoadNormalHeightData(const std::filesystem::path& heightMapFileName);
		void GenerateMesh(const std::filesystem::path& terrainFileName,
			const std::filesystem::path& blendMapFileName, ID3D12GraphicsCommandList* commandList,
			Graphics::DirectX12Renderer* renderer);
		
		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const TerrainDesc& desc);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const TerrainDesc& desc);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const TerrainDesc& desc);

		void CreateMaterial(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const TerrainDesc& desc);

		void LoadCache(const std::filesystem::path& fileName, ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager);
		void SaveCache(const std::filesystem::path& fileName, const Graphics::Assets::MeshDesc& meshDesc,
			const std::vector<uint8_t>& verticesData, const std::vector<uint8_t>& indicesData) const;

		void FetchCoord(const float2& position, uint32_t& index00, uint32_t& index10,
			uint32_t& index01, uint32_t& index11, float4& barycentricCoord) const;

		template<typename T>
		void FillIndices(T* indicesPtr, uint32_t indicesNumber)
		{
			auto cellsPerWidth = verticesPerWidth - 1;
			auto cellsPerHeight = verticesPerHeight - 1;

			for (uint32_t cellIndexY = 0u; cellIndexY < cellsPerHeight; cellIndexY++)
				for (uint32_t cellIndexX = 0u; cellIndexX < cellsPerWidth; cellIndexX++)
				{
					auto startIndex = cellIndexX + cellIndexY * verticesPerWidth;

					*indicesPtr = startIndex;
					indicesPtr++;

					*indicesPtr = startIndex + 1u;
					indicesPtr++;

					*indicesPtr = startIndex + verticesPerWidth;
					indicesPtr++;

					*indicesPtr = startIndex + verticesPerWidth;
					indicesPtr++;

					*indicesPtr = startIndex + 1u;
					indicesPtr++;

					*indicesPtr = startIndex + verticesPerWidth + 1u;
					indicesPtr++;
				}
		}

		struct TerrainVertex
		{
			float3 position;
			DirectX::PackedVector::HALF normalX;
			DirectX::PackedVector::HALF normalY;
			DirectX::PackedVector::HALF normalZ;
			DirectX::PackedVector::HALF normalW;
			DirectX::PackedVector::HALF tangentX;
			DirectX::PackedVector::HALF tangentY;
			DirectX::PackedVector::HALF tangentZ;
			DirectX::PackedVector::HALF tangentW;
			DirectX::PackedVector::HALF texCoordX;
			DirectX::PackedVector::HALF texCoordY;
		};

		struct MutableConstants
		{
		public:
			float4x4 viewProjection;

			float3 cameraPosition;
			float time;

			float2 mapTiling0;
			float2 mapTiling1;
			float2 mapTiling2;
			float2 mapTiling3;

			float zNear;
			float zFar;
			float mipBias;
			float padding;

			float4x4 lastViewProjection;
		};

		struct DepthPassConstants
		{
			float4x4 world;
			uint32_t lightMatrixStartIndex;
		};

		DepthPassConstants depthPassConstants;

		std::vector<floatN> normalHeightData;

		uint32_t verticesPerWidth;
		uint32_t verticesPerHeight;
		
		float3 minCorner;
		float3 mapSize;

		bool hasDepthPass;
		bool hasDepthPassCube;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Resources::ResourceID lightConstantBufferId;
		Graphics::Resources::ResourceID mutableConstantsId;

		Graphics::Resources::ResourceID albedo0Id;
		Graphics::Resources::ResourceID albedo1Id;
		Graphics::Resources::ResourceID albedo2Id;
		Graphics::Resources::ResourceID albedo3Id;
		Graphics::Resources::ResourceID normal0Id;
		Graphics::Resources::ResourceID normal1Id;
		Graphics::Resources::ResourceID normal2Id;
		Graphics::Resources::ResourceID normal3Id;
		Graphics::Resources::ResourceID blendMapId;

		Graphics::Resources::ResourceID terrainVSId;
		Graphics::Resources::ResourceID terrainPSId;

		Graphics::Assets::Mesh* mesh;
		Graphics::Assets::Material* material;
		Graphics::Assets::Material* materialDepthPrepass;
		Graphics::Assets::Material* materialDepthPass;
		Graphics::Assets::Material* materialDepthCubePass;
	};
}
