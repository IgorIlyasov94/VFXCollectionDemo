#pragma once

#include "../DirectX12Includes.h"

namespace Graphics::Assets
{
	class GeometryUtilities final
	{
	public:
		static float CalculateTriangleArea(float3 point0, float3 point1, float3 point2) noexcept;

		static float3 CalculateNormal(const float3& point0, const float3& point1, const float3& point2) noexcept;
		static float3 CalculateTangent(const float3& normal) noexcept;
		static float3 CalculateBarycentric(float3 point0, float3 point1, float3 point2, float3 point) noexcept;

		static bool PointInTriangle(const float3& point0, const float3& point1, const float3& point2, const float3& point) noexcept;

		static void TriangulatePolygon(const std::vector<uint8_t>& vertexBuffer, size_t stride, std::vector<uint32_t>& vertexIndices);
		static void RecalculateNormals(const std::vector<uint32_t>& vertexIndices, size_t stride, std::vector<uint8_t>& vertexBuffer);
		static void CalculateTangents(size_t stride, std::vector<uint8_t>& vertexBuffer);

		static constexpr float EPSILON = 1E-5f;

	private:
		GeometryUtilities() = delete;
		~GeometryUtilities() = delete;
		GeometryUtilities(const GeometryUtilities&) = delete;
		GeometryUtilities(GeometryUtilities&&) = delete;
		GeometryUtilities& operator=(const GeometryUtilities&) = delete;
		GeometryUtilities& operator=(GeometryUtilities&&) = delete;
	};
}
