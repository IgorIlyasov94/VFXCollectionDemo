#pragma once

#include "../MeshDesc.h"

namespace Graphics::Assets::Loaders
{
	class OBJLoader final
	{
	public:
		static void Load(std::filesystem::path filePath, bool recalculateNormals, bool addTangents,
			MeshDesc& meshDesc, std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData);

	private:
		OBJLoader() = delete;
		~OBJLoader() = delete;
		OBJLoader(const OBJLoader&) = delete;
		OBJLoader(OBJLoader&&) = delete;
		OBJLoader& operator=(const OBJLoader&) = delete;
		OBJLoader& operator=(OBJLoader&&) = delete;

		enum class TokenType : uint8_t
		{
			NO_TOKEN = 0u,
			POSITION = 1u,
			NORMAL = 2u,
			TEXCOORD = 3u,
			FACE = 4u
		};

		struct TempGeometryData
		{
			const std::vector<float3>& positions;
			const std::vector<float3>& normals;
			const std::vector<float2>& texCoords;
			const std::vector<std::array<int32_t, 3>>& face;
		};

		static TokenType GetToken(std::stringstream& objLineStream);
		static float2 GetVector2(std::stringstream& objLineStream);
		static float3 GetVector3(std::stringstream& objLineStream);

		static void GetFace(std::stringstream& objLineStream, VertexFormat vertexFormat, std::vector<uint32_t>& positionFace,
			std::vector<uint32_t>& normalFace, std::vector<uint32_t>& texCoordFace);

		static VertexFormat GetVertexFormat(size_t texCoordCount, size_t normalCount);

		static void AppendToBuffer(const TempGeometryData& tempGeometryData, bool recalculateNormals, bool addTangents,
			std::vector<uint8_t>& vertexBufferData);
		static uint32_t CalculateLocalStride(VertexFormat format);

		static void PushPositionToBuffer(const float3& value, std::vector<uint8_t>& vertexBufferData);
		static void PushNormalToBuffer(const float3& value, std::vector<uint8_t>& vertexBufferData);
		static void PushTexCoordToBuffer(const float2& value, std::vector<uint8_t>& vertexBufferData);
	};
}
