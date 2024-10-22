#include "GeneratorUtilities.h"

using namespace DirectX;

void Graphics::Assets::Generators::GeneratorUtilities::Normalize(const std::vector<floatN>& map, std::vector<float4>& result)
{
	floatN minValue{};
	floatN maxValue{};
	FindMinMax(map, minValue, maxValue);

	auto diffValue = XMVectorSubtract(maxValue, minValue);

	if (result.empty())
		result.resize(map.size(), {});

	for (auto pixelIndex = 0u; pixelIndex < map.size(); pixelIndex++)
		Normalize(map[pixelIndex], minValue, diffValue, result[pixelIndex]);
}

uint32_t Graphics::Assets::Generators::GeneratorUtilities::GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z,
	uint32_t width, uint32_t height)
{
	return static_cast<uint32_t>(width * (static_cast<uint64_t>(z) * height + y) + x);
}

void Graphics::Assets::Generators::GeneratorUtilities::GetXYZFromIndex(uint32_t index, uint32_t width, uint32_t height,
	uint32_t& x, uint32_t& y, uint32_t& z)
{
	x = index % width;
	y = (index / width) % height;
	z = (index / width) / height;
}

void Graphics::Assets::Generators::GeneratorUtilities::Normalize(const floatN& value, const floatN& minValue,
	const floatN& diffValue, float4& result)
{
	XMStoreFloat4(&result, XMVectorDivide(XMVectorSubtract(value, minValue), diffValue));
}

void Graphics::Assets::Generators::GeneratorUtilities::FindMinMax(const std::vector<floatN>& map,
	floatN& minValue, floatN& maxValue)
{
	minValue = map[0];
	maxValue = map[0];

	for (auto& element : map)
	{
		minValue = XMVectorMin(minValue, element);
		maxValue = XMVectorMax(maxValue, element);
	}
}
