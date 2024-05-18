#include "GeometryUtilities.h"

using namespace DirectX;

float Graphics::Assets::GeometryUtilities::CalculateTriangleArea(float3 point0, float3 point1, float3 point2) noexcept
{
	floatN vector0_1 = XMLoadFloat3(&point1) - XMLoadFloat3(&point0);
	auto vector1_2 = XMLoadFloat3(&point2) - XMLoadFloat3(&point1);

	auto triangleVectorArea = XMVector3Cross(vector0_1, vector1_2);

	float result{};
	XMStoreFloat(&result, XMVector3Dot(triangleVectorArea, triangleVectorArea));

	return std::sqrt(result);
}

float3 Graphics::Assets::GeometryUtilities::CalculateNormal(const float3& point0, const float3& point1, const float3& point2) noexcept
{
	auto vector0_1 = XMLoadFloat3(&point1) - XMLoadFloat3(&point0);
	auto vector1_2 = XMLoadFloat3(&point2) - XMLoadFloat3(&point0);

	auto normal = XMVector3Cross(vector0_1, vector1_2);
	normal = XMVector3Normalize(normal);

	float3 result;
	XMStoreFloat3(&result, normal);

	return result;
}

float3 Graphics::Assets::GeometryUtilities::CalculateBarycentric(float3 point0, float3 point1, float3 point2, float3 point) noexcept
{
	float triangleArea = CalculateTriangleArea(point0, point1, point2);

	float3 result{};
	result.x = CalculateTriangleArea(point1, point2, point) / triangleArea;
	result.y = CalculateTriangleArea(point2, point0, point) / triangleArea;
	result.z = CalculateTriangleArea(point0, point1, point) / triangleArea;

	return result;
}

bool Graphics::Assets::GeometryUtilities::PointInTriangle(const float3& point0, const float3& point1, const float3& point2,
	const float3& point) noexcept
{
	float3 barycentric = CalculateBarycentric(point0, point1, point2, point);

	if (barycentric.x + barycentric.y + barycentric.z <= 1.0f)
		return true;

	return false;
}

void Graphics::Assets::GeometryUtilities::TriangulatePolygon(const std::vector<uint8_t>& vertexBuffer, size_t stride,
	std::vector<uint32_t>& vertexIndices)
{
	auto polygonSize = vertexIndices.size();
	std::vector<uint32_t> triangles;

	if (polygonSize <= 2)
	{
		vertexIndices.clear();
		return;
	}

	if (polygonSize == 3)
	{
		vertexIndices = { 0, 1, 2 };
		return;
	}

	triangles.reserve(polygonSize * 2);

	std::vector<uint32_t> tempIndices = vertexIndices;

	for (int32_t iterator = 0; iterator < tempIndices.size(); iterator++)
	{
		auto indicesCount = tempIndices.size();
		int32_t index0 = static_cast<int32_t>((indicesCount + iterator - 1) % indicesCount);
		int32_t index1 = iterator;
		int32_t index2 = static_cast<int32_t>((iterator + 1ui64) % indicesCount);

		if (index0 > index1)
		{
			std::swap(index0, index1);
			std::swap(index1, index2);
		}
		else if (index1 > index2)
		{
			std::swap(index1, index2);
			std::swap(index0, index1);
		}

		auto previousIndex = tempIndices[index0];
		auto currentIndex = tempIndices[index1];
		auto nextIndex = tempIndices[index2];

		auto& previousPosition = reinterpret_cast<const float3&>(vertexBuffer[previousIndex * stride]);
		auto& currentPosition = reinterpret_cast<const float3&>(vertexBuffer[currentIndex * stride]);
		auto& nextPosition = reinterpret_cast<const float3&>(vertexBuffer[nextIndex * stride]);

		if (indicesCount == 3)
		{
			triangles.push_back(previousIndex);
			triangles.push_back(currentIndex);
			triangles.push_back(nextIndex);

			break;
		}
		else if (indicesCount == 4)
		{
			triangles.push_back(previousIndex);
			triangles.push_back(currentIndex);
			triangles.push_back(nextIndex);

			for (auto& index : tempIndices)
				if (index != previousIndex && index != currentIndex && index != nextIndex)
				{
					triangles.push_back(nextIndex);
					triangles.push_back(index);
					triangles.push_back(previousIndex);

					break;
				}

			break;
		}

		auto v0 = XMLoadFloat3(&previousPosition) - XMLoadFloat3(&currentPosition);
		auto v1 = XMLoadFloat3(&nextPosition) - XMLoadFloat3(&currentPosition);
		auto dotV0V1 = XMVector3Dot(v0, v1);
		
		float cosAngle{};
		XMStoreFloat(&cosAngle, dotV0V1);

		float denominator{};
		auto v0v1 = XMVector3Length(v0) * XMVector3Length(v1);
		XMStoreFloat(&denominator, v0v1);

		cosAngle /= denominator;

		if (cosAngle <= 0.0f || cosAngle >= 1.0f)
			continue;

		auto inTriangle = false;

		for (int32_t iterator1 = 0; iterator1 < tempIndices.size(); iterator1++)
		{
			auto index = tempIndices[iterator1];

			if (index != previousIndex && index != currentIndex && index != nextIndex)
			{
				auto& point = reinterpret_cast<const float3&>(vertexBuffer[index * stride]);

				if (PointInTriangle(previousPosition, currentPosition, nextPosition, point))
				{
					inTriangle = true;

					break;
				}
			}
		}

		if (inTriangle)
			continue;

		triangles.push_back(previousIndex);
		triangles.push_back(currentIndex);
		triangles.push_back(nextIndex);

		tempIndices.erase(tempIndices.begin() + iterator);

		iterator = -1;
	}

	vertexIndices = triangles;
}

void Graphics::Assets::GeometryUtilities::RecalculateNormals(const std::vector<uint32_t>& vertexIndices, size_t stride,
	std::vector<uint8_t>& vertexBuffer)
{
	for (size_t triangleIndex = 0; triangleIndex < vertexIndices.size() / 3; triangleIndex++)
	{
		auto& index0 = vertexIndices[triangleIndex * 3];
		auto& index1 = vertexIndices[triangleIndex * 3 + 1];
		auto& index2 = vertexIndices[triangleIndex * 3 + 2];

		auto vertexIndex0 = index0 * stride;
		auto vertexIndex1 = index1 * stride;
		auto vertexIndex2 = index2 * stride;

		auto& position0 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex0]);
		auto& position1 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex1]);
		auto& position2 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex2]);

		auto normal = CalculateNormal(position0, position1, position2);

		auto& resultBufferSlot = reinterpret_cast<float3&>(vertexBuffer[vertexIndex0 + 12]);

		resultBufferSlot = normal;
	}
}
