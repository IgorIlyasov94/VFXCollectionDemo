#include "TurbulenceMapGenerator.h"
#include "NoiseGenerator.h"
#include "GeneratorUtilities.h"

using namespace DirectX;

Graphics::Assets::Generators::TurbulenceMapGenerator::TurbulenceMapGenerator()
{

}

Graphics::Assets::Generators::TurbulenceMapGenerator::~TurbulenceMapGenerator()
{

}

void Graphics::Assets::Generators::TurbulenceMapGenerator::Generate(uint32_t width, uint32_t height, uint32_t depth,
	const float3& scale, std::vector<floatN>& textureData)
{
	NoiseGenerator noiseGenerator{};
	noiseGenerator.Generate(width, height, depth, scale, textureData);

	std::vector<floatN> forceMap;
	forceMap.resize(textureData.size());

	Rotor(width, height, depth, textureData, forceMap);

	auto maxLength = FindMaxRotorLength(forceMap);
	FitRotorLength(maxLength, forceMap);

	std::vector<floatN> temp;
	temp.resize(textureData.size());

	GeneratorUtilities::GaussianBlur(width, height, depth, 0, BLUR_MAX_SIZE, forceMap, textureData, temp);

	SmoothMap(width, height, depth, temp, textureData);
}

void Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(uint32_t width, uint32_t height, uint32_t depth,
	const std::vector<floatN>& map, std::vector<floatN>& result)
{
	for (uint32_t zIndex = 0u; zIndex < depth; zIndex++)
	{
		for (uint32_t yIndex = 0u; yIndex < height; yIndex++)
		{
			for (uint32_t xIndex = 0u; xIndex < width; xIndex++)
			{
				auto pixelIndex = GeneratorUtilities::GetIndexFromXYZ(xIndex, yIndex, zIndex, width, height);
				auto pixelIndexX = GeneratorUtilities::GetIndexFromXYZ((xIndex + 1) % width, yIndex, zIndex, width, height);
				auto pixelIndexY = GeneratorUtilities::GetIndexFromXYZ(xIndex, (yIndex + 1) % height, zIndex, width, height);
				auto pixelIndexZ = GeneratorUtilities::GetIndexFromXYZ(xIndex, yIndex, (zIndex + 1) % depth, width, height);

				result[pixelIndex] = Rotor(map[pixelIndex], map[pixelIndexX], map[pixelIndexY], map[pixelIndexZ]);
			}
		}
	}
}

float Graphics::Assets::Generators::TurbulenceMapGenerator::FindMaxRotorLength(const std::vector<floatN>& map)
{
	float maxLength = 1E-5f;

	for (const auto& value : map)
	{
		auto length = XMVector3Length(value).m128_f32[0];

		if (maxLength < length)
			maxLength = length;
	}

	return maxLength;
}

void Graphics::Assets::Generators::TurbulenceMapGenerator::FitRotorLength(float maxLength, std::vector<floatN>& map)
{
	auto rcpLength = XMVectorReplicate(ROTOR_MULTIPLIER / maxLength);

	for (auto& value : map)
		value = XMVectorMultiply(value, rcpLength);
}

void Graphics::Assets::Generators::TurbulenceMapGenerator::SmoothMap(uint32_t width, uint32_t height, uint32_t depth,
	std::vector<floatN>& map, std::vector<floatN>& result)
{
	auto halfSamplesNumberX = static_cast<int32_t>(std::round(SMOOTH_FILTER_MAX_SIZE * width / BASE_MAP_SIZE));
	auto halfSamplesNumberY = static_cast<int32_t>(std::round(SMOOTH_FILTER_MAX_SIZE * height / BASE_MAP_SIZE));
	auto halfSamplesNumberZ = static_cast<int32_t>(std::round(SMOOTH_FILTER_MAX_SIZE * depth / BASE_MAP_SIZE));

	halfSamplesNumberX = std::max(halfSamplesNumberX, 1);
	halfSamplesNumberY = std::max(halfSamplesNumberY, 1);
	halfSamplesNumberZ = std::max(halfSamplesNumberZ, 1);

	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberX, halfSamplesNumberX,
		float3(1.0f, 0.0f, 0.0f), map, result);

	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberY, halfSamplesNumberY,
		float3(0.0f, 1.0f, 0.0f), result, map);

	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberZ, halfSamplesNumberZ,
		float3(0.0f, 0.0f, 1.0f), map, result);

	GeneratorUtilities::Normalize(result);
}

floatN Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(const floatN& w_0,
	const floatN& w_x1, const floatN& w_y1, const floatN& w_z1)
{
	auto dXdY = w_y1.m128_f32[0] - w_0.m128_f32[0];
	auto dXdZ = w_z1.m128_f32[0] - w_0.m128_f32[0];
	auto dYdX = w_x1.m128_f32[1] - w_0.m128_f32[1];
	auto dYdZ = w_z1.m128_f32[1] - w_0.m128_f32[1];
	auto dZdX = w_x1.m128_f32[2] - w_0.m128_f32[2];
	auto dZdY = w_y1.m128_f32[2] - w_0.m128_f32[2];
	
	return XMVectorSet(dZdY - dYdZ, dXdZ - dZdX, dYdX - dXdY, 1.0f);
}
