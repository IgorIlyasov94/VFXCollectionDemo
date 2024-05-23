#include "OBJLoader.h"
#include "../GeometryUtilities.h"

void Graphics::Assets::Loaders::OBJLoader::Load(std::filesystem::path filePath, bool recalculateNormals, bool addTangents,
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

		if (token == TokenType::NO_TOKEN)
			continue;

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

				if ((recalculateNormals || addTangents) && ((vertexFormat & VertexFormat::NORMAL) != VertexFormat::NORMAL))
					stride += 8u;

				if (addTangents)
					stride += 8u;
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

			AppendToBuffer(tempGeometryData, recalculateNormals, addTangents, verticesData);
			GeometryUtilities::TriangulatePolygon(verticesData, stride, currentIndices);

			resultIndices.insert(resultIndices.end(), currentIndices.begin(), currentIndices.end());
		}
	}

	if (recalculateNormals || addTangents)
	{
		if ((vertexFormat & VertexFormat::NORMAL) != VertexFormat::NORMAL)
		{
			vertexFormat |= VertexFormat::NORMAL;
			GeometryUtilities::RecalculateNormals(resultIndices, stride, verticesData);
		}
		else if (recalculateNormals)
			GeometryUtilities::RecalculateNormals(resultIndices, stride, verticesData);
	}

	if (addTangents)
	{
		if ((vertexFormat & VertexFormat::TANGENT) != VertexFormat::TANGENT)
			vertexFormat |= VertexFormat::TANGENT;

		GeometryUtilities::CalculateTangents(stride, verticesData);
	}

	meshDesc.vertexFormat = vertexFormat;

	auto verticesNumber = static_cast<uint32_t>(verticesData.size() / stride);
	meshDesc.verticesNumber = verticesNumber;
	meshDesc.indicesNumber = static_cast<uint32_t>(resultIndices.size());

	uint32_t indexStride = verticesNumber <= std::numeric_limits<uint16_t>::max() ? 2u : 4u;
	meshDesc.indexFormat = indexStride == 2u ? IndexFormat::UINT16_INDEX : IndexFormat::UINT32_INDEX;

	indicesData.resize(resultIndices.size() * indexStride);

	auto indicesDataPtr = indicesData.data();

	for (auto& index : resultIndices)
	{
		if (indexStride == 2u)
			reinterpret_cast<uint16_t*>(indicesDataPtr)[0] = static_cast<uint16_t>(index);
		else
			reinterpret_cast<uint32_t*>(indicesDataPtr)[0] = index;

		indicesDataPtr += indexStride;
	}

	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

Graphics::Assets::Loaders::OBJLoader::TokenType Graphics::Assets::Loaders::OBJLoader::GetToken(std::stringstream& objLineStream)
{
	std::string strToken;
	objLineStream >> strToken;

	auto token = TokenType::NO_TOKEN;

	if (strToken == "v")
		token = TokenType::POSITION;
	else if (strToken == "vn")
		token = TokenType::NORMAL;
	else if (strToken == "vt")
		token = TokenType::TEXCOORD;
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

void Graphics::Assets::Loaders::OBJLoader::AppendToBuffer(const TempGeometryData& tempGeometryData, bool recalculateNormals,
	bool addTangents, std::vector<uint8_t>& vertexBufferData)
{
	for (auto& vertexIndices : tempGeometryData.face)
	{
		if (vertexIndices[0] < 0)
			continue;

		auto& position = tempGeometryData.positions[vertexIndices[0]];
		PushPositionToBuffer(position, vertexBufferData);

		if (vertexIndices[1] >= 0 && !recalculateNormals)
		{
			auto& normal = tempGeometryData.normals[vertexIndices[1]];
			PushNormalToBuffer(normal, vertexBufferData);
		}
		else if (recalculateNormals || addTangents)
		{
			for (auto iterator = 0u; iterator < sizeof(DirectX::PackedVector::XMHALF4); iterator++)
				vertexBufferData.push_back({});
		}

		if (addTangents)
			for (auto iterator = 0u; iterator < sizeof(DirectX::PackedVector::XMHALF4); iterator++)
				vertexBufferData.push_back({});

		if (vertexIndices[2] >= 0)
		{
			auto& texCoord = tempGeometryData.texCoords[vertexIndices[2]];
			PushTexCoordToBuffer(texCoord, vertexBufferData);
		}
	}
}

uint32_t Graphics::Assets::Loaders::OBJLoader::CalculateLocalStride(VertexFormat format)
{
	uint32_t result = 12u;

	if ((format & VertexFormat::NORMAL) == VertexFormat::NORMAL)
		result += 12u;

	if ((format & VertexFormat::TEXCOORD0) == VertexFormat::TEXCOORD0)
		result += 8u;

	return result;
}

void Graphics::Assets::Loaders::OBJLoader::PushPositionToBuffer(const float3& value, std::vector<uint8_t>& vertexBufferData)
{
	auto offset = vertexBufferData.size();

	for (auto iterator = 0u; iterator < sizeof(float3); iterator++)
		vertexBufferData.push_back({});

	auto address0 = reinterpret_cast<float*>(vertexBufferData.data() + offset);
	auto address1 = address0 + 1u;
	auto address2 = address1 + 1u;

	*address0 = value.x;
	*address1 = value.y;
	*address2 = value.z;
}

void Graphics::Assets::Loaders::OBJLoader::PushNormalToBuffer(const float3& value, std::vector<uint8_t>& vertexBufferData)
{
	auto offset = vertexBufferData.size();

	for (auto iterator = 0u; iterator < sizeof(DirectX::PackedVector::XMHALF4); iterator++)
		vertexBufferData.push_back({});

	auto address0 = reinterpret_cast<DirectX::PackedVector::HALF*>(vertexBufferData.data() + offset);
	auto address1 = address0 + 1u;
	auto address2 = address1 + 1u;
	auto address3 = address2 + 1u;

	*address0 = DirectX::PackedVector::XMConvertFloatToHalf(value.x);
	*address1 = DirectX::PackedVector::XMConvertFloatToHalf(value.y);
	*address2 = DirectX::PackedVector::XMConvertFloatToHalf(value.z);
	*address3 = 0u;
}

void Graphics::Assets::Loaders::OBJLoader::PushTexCoordToBuffer(const float2& value, std::vector<uint8_t>& vertexBufferData)
{
	auto offset = vertexBufferData.size();

	for (auto iterator = 0u; iterator < sizeof(DirectX::PackedVector::XMHALF2); iterator++)
		vertexBufferData.push_back({});

	auto address0 = reinterpret_cast<DirectX::PackedVector::HALF*>(vertexBufferData.data() + offset);
	auto address1 = address0 + 1u;

	*address0 = DirectX::PackedVector::XMConvertFloatToHalf(value.x);
	*address1 = DirectX::PackedVector::XMConvertFloatToHalf(value.y);
}
