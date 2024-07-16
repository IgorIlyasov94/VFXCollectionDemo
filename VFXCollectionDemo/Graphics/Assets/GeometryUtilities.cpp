#include "GeometryUtilities.h"

using namespace DirectX;

float Graphics::Assets::GeometryUtilities::CalculateTriangleArea(float3 point0, float3 point1, float3 point2) noexcept
{
	floatN vector0_1 = XMLoadFloat3(&point1) - XMLoadFloat3(&point0);
	auto vector1_2 = XMLoadFloat3(&point2) - XMLoadFloat3(&point1);

	auto triangleVectorArea = XMVector3Cross(vector0_1, vector1_2);

	float result{};
	XMStoreFloat(&result, XMVector3Dot(triangleVectorArea, triangleVectorArea));

	return std::sqrt(result) * 0.5f;
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

float3 Graphics::Assets::GeometryUtilities::CalculateTangent(const float3& normal) noexcept
{
	float3 v0(0.0f, 0.0f, 1.0f);
	float3 v1(0.0f, 1.0f, 0.0f);

	floatN t0 = XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&v0));
	floatN t1 = XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&v1));

	float3 tangent{};

	if (XMVector3Length(t0).m128_f32[0] > XMVector3Length(t1).m128_f32[0])
		XMStoreFloat3(&tangent, XMVector3Normalize(t0));
	else
		XMStoreFloat3(&tangent, XMVector3Normalize(t1));

	return tangent;
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

	if (barycentric.x + barycentric.y + barycentric.z <= (1.0f + EPSILON * 3.0f))
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
		return;

	if (polygonSize == 4)
	{
		vertexIndices.resize(6u);
		vertexIndices[4] = vertexIndices[3];
		vertexIndices[3] = vertexIndices[2];
		vertexIndices[5] = vertexIndices[0];

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
	for (size_t triangleIndex = 0u; triangleIndex < vertexIndices.size() / 3; triangleIndex++)
	{
		auto vertexIndexStart = triangleIndex * 3;

		auto& index0 = vertexIndices[vertexIndexStart];
		auto& index1 = vertexIndices[vertexIndexStart + 1];
		auto& index2 = vertexIndices[vertexIndexStart + 2];

		auto vertexIndex0 = index0 * stride;
		auto vertexIndex1 = index1 * stride;
		auto vertexIndex2 = index2 * stride;

		auto& position0 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex0]);
		auto& position1 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex1]);
		auto& position2 = reinterpret_cast<const float3&>(vertexBuffer[vertexIndex2]);

		auto normal = CalculateNormal(position0, position1, position2);

		DirectX::PackedVector::XMHALF4 half4Normal{};
		half4Normal.x = DirectX::PackedVector::XMConvertFloatToHalf(normal.x);
		half4Normal.y = DirectX::PackedVector::XMConvertFloatToHalf(normal.y);
		half4Normal.z = DirectX::PackedVector::XMConvertFloatToHalf(normal.z);

		auto& resultBufferSlot = reinterpret_cast<DirectX::PackedVector::XMHALF4&>(vertexBuffer[vertexIndex0 + 12u]);
		resultBufferSlot = half4Normal;
	}
}

void Graphics::Assets::GeometryUtilities::CalculateTangents(size_t stride, std::vector<uint8_t>& vertexBuffer)
{
	auto vertexNumber = vertexBuffer.size() / stride;

	for (size_t vertexIndex = 0u; vertexIndex < vertexNumber; vertexIndex++)
	{
		auto vertexBufferNormalIndex = vertexIndex * stride + 12u;
		auto vertexBufferTangetIndex = vertexBufferNormalIndex + 8u;

		auto& half4Normal = reinterpret_cast<const DirectX::PackedVector::XMHALF4&>(vertexBuffer[vertexBufferNormalIndex]);
		float3 normal{};
		normal.x = DirectX::PackedVector::XMConvertHalfToFloat(half4Normal.x);
		normal.y = DirectX::PackedVector::XMConvertHalfToFloat(half4Normal.y);
		normal.z = DirectX::PackedVector::XMConvertHalfToFloat(half4Normal.z);

		auto tangent = CalculateTangent(normal);
		DirectX::PackedVector::XMHALF4 half4Tangent{};
		half4Tangent.x = DirectX::PackedVector::XMConvertFloatToHalf(tangent.x);
		half4Tangent.y = DirectX::PackedVector::XMConvertFloatToHalf(tangent.y);
		half4Tangent.z = DirectX::PackedVector::XMConvertFloatToHalf(tangent.z);

		auto& resultBufferSlot = reinterpret_cast<DirectX::PackedVector::XMHALF4&>(vertexBuffer[vertexBufferTangetIndex]);
		resultBufferSlot = half4Tangent;
	}
}
