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

	std::vector<floatN> temp;
	temp.resize(textureData.size());

	GeneratorUtilities::GaussianBlur(width, height, depth, 0, BLUR_MAX_SIZE - 1, forceMap, textureData, temp);

	GeneratorUtilities::Normalize(temp, textureData);
}

void Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(uint32_t width, uint32_t height, uint32_t depth,
	const std::vector<floatN>& textureData, std::vector<floatN>& result)
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

				result[pixelIndex] = Rotor(textureData[pixelIndex], textureData[pixelIndexX],
					textureData[pixelIndexY], textureData[pixelIndexZ]);
			}
		}
	}
}

floatN Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(const floatN& w_0,
	const floatN& w_x1, const floatN& w_y1, const floatN& w_z1)
{
	auto dXdY = (w_y1.m128_f32[0] - w_0.m128_f32[0]) * ROTOR_SCALE;
	auto dXdZ = (w_z1.m128_f32[0] - w_0.m128_f32[0]) * ROTOR_SCALE;
	auto dYdX = (w_x1.m128_f32[1] - w_0.m128_f32[1]) * ROTOR_SCALE;
	auto dYdZ = (w_z1.m128_f32[1] - w_0.m128_f32[1]) * ROTOR_SCALE;
	auto dZdX = (w_x1.m128_f32[2] - w_0.m128_f32[2]) * ROTOR_SCALE;
	auto dZdY = (w_y1.m128_f32[2] - w_0.m128_f32[2]) * ROTOR_SCALE;

	return XMVectorSet(dZdY - dYdZ, dXdZ - dZdX, dYdX - dXdY, 1.0f);
}
