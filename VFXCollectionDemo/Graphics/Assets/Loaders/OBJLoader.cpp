#include "OBJLoader.h"
#include "../GeometryUtilities.h"

void Graphics::Assets::Loaders::OBJLoader::Load(std::filesystem::path filePath, bool recalculateNormals,
	MeshDesc& meshDesc, std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData)
{
	std::ifstream objFile(filePath, std::ios::in);

	std::vector<float3> positions;
	std::vector<float3> normals;
	std::vector<float2> texCoords;
	std::vector<uint32_t> positionFace;
	std::vector<uint32_t> normalFace;
	std::vector<uint32_t> texCoordFace;
	std::vector<std::array<int32_t, 3>> currentFace;
	std::vector<std::array<int32_t, 3>> faces;
	std::vector<uint32_t> currentIndices;
	std::vector<uint32_t> resultIndices;

	TempGeometryData tempGeometryData
	{
		positions,
		normals,
		texCoords,
		currentFace
	};

	std::string objLine;
	VertexFormat vertexFormat = VertexFormat::POSITION;
	uint32_t stride = 12u;
	verticesData.clear();
	indicesData.clear();

	while (std::getline(objFile, objLine))
	{
		std::stringstream objLineStream(objLine);

		auto token = GetToken(objLineStream);

		if (token == TokenType::POSITION)
			positions.push_back(GetVector3(objLineStream));
		else if (token == TokenType::NORMAL)
			normals.push_back(GetVector3(objLineStream));
		else if (token == TokenType::TEXCOORD)
			texCoords.push_back(GetVector2(objLineStream));
		else
		{
			if (verticesData.empty())
			{
				vertexFormat = GetVertexFormat(texCoords.size(), normals.size());
				stride = VertexStride(vertexFormat);

				if (recalculateNormals && ((vertexFormat & VertexFormat::NORMAL) != VertexFormat::NORMAL))
					stride += 12u;
			}

			positionFace.clear();
			normalFace.clear();
			texCoordFace.clear();
			currentIndices.clear();
			currentFace.clear();

			GetFace(objLineStream, vertexFormat, positionFace, normalFace, texCoordFace);

			for (uint32_t index = 0; index < positionFace.size(); index++)
			{
				auto positionIndex = positionFace[index];
				auto normalIndex = normalFace.empty() ? -1 : normalFace[index];
				auto texCoordIndex = texCoordFace.empty() ? -1 : texCoordFace[index];

				std::array<int32_t, 3> face =
				{
					static_cast<int32_t>(positionIndex),
					static_cast<int32_t>(normalIndex),
					static_cast<int32_t>(texCoordIndex)
				};

				auto predicate = [&face](std::array<int32_t, 3> i)
					{
						return i[0] == face[0] && i[1] == face[1] && i[2] == face[2];
					};

				if (auto it = std::find_if(faces.begin(), faces.end(), predicate); it != std::end(faces))
				{
					currentIndices.push_back(static_cast<uint32_t>(it - faces.begin()));
					currentFace.push_back({ -1, -1, -1 });
				}
				else
				{
					currentIndices.push_back(static_cast<uint32_t>(faces.size()));
					faces.push_back(face);
					currentFace.push_back(face);
				}
			}

			AppendToBuffer(tempGeometryData, verticesData);
			GeometryUtilities::TriangulatePolygon(verticesData, stride, currentIndices);

			resultIndices.insert(resultIndices.end(), currentIndices.begin(), currentIndices.end());
		}
	}

	if (recalculateNormals)
	{
		vertexFormat |= VertexFormat::NORMAL;
		GeometryUtilities::RecalculateNormals(resultIndices, stride, verticesData);
	}

	meshDesc.vertexFormat = vertexFormat;

	auto verticesNumber = static_cast<uint32_t>(verticesData.size() * sizeof(float) / (stride));
	meshDesc.verticesNumber = verticesNumber;
	meshDesc.indicesNumber = static_cast<uint32_t>(resultIndices.size() * 2u);

	uint32_t shift = 0u;

	if (verticesNumber <= std::numeric_limits<uint16_t>::max())
	{
		shift = 2u;
		meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	}
	else
	{
		shift = 4u;
		meshDesc.indexFormat = IndexFormat::UINT32_INDEX;
	}

	indicesData.resize(resultIndices.size() * shift);

	auto indicesDataPtr = indicesData.data();

	for (auto& index : resultIndices)
	{
		if (shift == 2u)
			reinterpret_cast<uint16_t*>(indicesDataPtr)[0] = static_cast<uint16_t>(index);
		else
			reinterpret_cast<uint32_t*>(indicesDataPtr)[0] = index;

		indicesDataPtr += shift;
	}

	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

Graphics::Assets::Loaders::OBJLoader::TokenType Graphics::Assets::Loaders::OBJLoader::GetToken(std::stringstream& objLineStream)
{
	std::string strToken;
	objLineStream >> strToken;

	auto token = TokenType::TEXCOORD;

	if (strToken == "v")
		token = TokenType::POSITION;
	else if (strToken == "vn")
		token = TokenType::NORMAL;
	else if (strToken == "f")
		token = TokenType::FACE;

	return token;
}

float2 Graphics::Assets::Loaders::OBJLoader::GetVector2(std::stringstream& objLineStream)
{
	float2 result{};

	objLineStream >> result.x >> result.y;

	return result;
}

float3 Graphics::Assets::Loaders::OBJLoader::GetVector3(std::stringstream& objLineStream)
{
	float3 result{};

	objLineStream >> result.x >> result.y >> result.z;

	return result;
}

void Graphics::Assets::Loaders::OBJLoader::GetFace(std::stringstream& objLineStream, VertexFormat vertexFormat,
	std::vector<uint32_t>& positionFace, std::vector<uint32_t>& normalFace, std::vector<uint32_t>& texCoordFace)
{
	while (!objLineStream.eof())
	{
		uint32_t attributeIndex;

		objLineStream >> attributeIndex;

		if (objLineStream.bad() || objLineStream.fail())
			break;

		positionFace.push_back(--attributeIndex);

		if (vertexFormat == VertexFormat::POSITION)
			continue;

		objLineStream.ignore(vertexFormat == (VertexFormat::POSITION | VertexFormat::NORMAL) ? 2 : 1);
		objLineStream >> attributeIndex;

		if (vertexFormat == (VertexFormat::POSITION | VertexFormat::NORMAL))
			normalFace.push_back(--attributeIndex);
		else
			texCoordFace.push_back(--attributeIndex);

		if (vertexFormat == (VertexFormat::POSITION | VertexFormat::NORMAL | VertexFormat::TEXCOORD0))
		{
			objLineStream.ignore(1);

			objLineStream >> attributeIndex;
			normalFace.push_back(--attributeIndex);
		}
	}
}

Graphics::VertexFormat Graphics::Assets::Loaders::OBJLoader::GetVertexFormat(size_t texCoordCount, size_t normalCount)
{
	auto faceFormat = VertexFormat::POSITION;

	if (normalCount > 0u)
		faceFormat |= VertexFormat::NORMAL;

	if (texCoordCount > 0u)
		faceFormat |= VertexFormat::TEXCOORD0;

	return faceFormat;
}

void Graphics::Assets::Loaders::OBJLoader::AppendToBuffer(const TempGeometryData& tempGeometryData, std::vector<uint8_t>& vertexBufferData)
{
	for (auto& vertexIndices : tempGeometryData.face)
	{
		if (vertexIndices[0] < 0)
			continue;

		auto& position = tempGeometryData.positions[vertexIndices[0]];

		PushVector3ToBuffer(position, vertexBufferData);

		if (vertexIndices[1] > 0)
		{
			auto& normal = tempGeometryData.normals[vertexIndices[1]];

			PushVector3ToBuffer(normal, vertexBufferData);
		}

		if (vertexIndices[2] > 0)
		{
			auto& texCoord = tempGeometryData.texCoords[vertexIndices[2]];

			PushVector2ToBuffer(texCoord, vertexBufferData);
		}
	}
}

void Graphics::Assets::Loaders::OBJLoader::PushVector2ToBuffer(const float2& value, std::vector<uint8_t>& vertexBufferData)
{
	auto address0 = reinterpret_cast<float*>(&vertexBufferData.back());
	auto address1 = reinterpret_cast<float*>(&vertexBufferData.back() + sizeof(float));

	vertexBufferData.resize(vertexBufferData.size() + sizeof(float2));

	*address0 = value.x;
	*address1 = value.y;
}

void Graphics::Assets::Loaders::OBJLoader::PushVector3ToBuffer(const float3& value, std::vector<uint8_t>& vertexBufferData)
{
	auto address0 = reinterpret_cast<float*>(&vertexBufferData.back());
	auto address1 = reinterpret_cast<float*>(&vertexBufferData.back() + sizeof(float));
	auto address2 = reinterpret_cast<float*>(&vertexBufferData.back() + sizeof(float) * 2u);

	vertexBufferData.resize(vertexBufferData.size() + sizeof(float3));

	*address0 = value.x;
	*address1 = value.y;
	*address2 = value.z;
}
