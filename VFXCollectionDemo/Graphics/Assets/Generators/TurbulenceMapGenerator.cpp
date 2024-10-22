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
	const float3& scale, std::vector<float4>& textureData)
{
	NoiseGenerator noiseGenerator{};
	noiseGenerator.Generate(width, height, depth, scale, textureData);

	std::vector<floatN> temp;
	temp.resize(textureData.size());

	Rotor(width, height, depth, textureData, temp);

	GeneratorUtilities::Normalize(temp, textureData);
}

void Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(uint32_t width, uint32_t height, uint32_t depth,
	const std::vector<float4>& textureData, std::vector<floatN>& result)
{
	for (int32_t zIndex = 0u; zIndex < depth; zIndex++)
	{
		for (int32_t yIndex = 0u; yIndex < height; yIndex++)
		{
			for (int32_t xIndex = 0u; xIndex < width; xIndex++)
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

floatN Graphics::Assets::Generators::TurbulenceMapGenerator::Rotor(const float4& w_0,
	const float4& w_x1, const float4& w_y1, const float4& w_z1)
{
	auto dXdY = w_y1.x - w_0.x;
	auto dXdZ = w_z1.x - w_0.x;
	auto dYdX = w_x1.y - w_0.y;
	auto dYdZ = w_z1.y - w_0.y;
	auto dZdX = w_x1.z - w_0.z;
	auto dZdY = w_y1.z - w_0.z;

	return XMVectorSet(dZdY - dYdZ, dXdZ - dZdX, dYdX - dXdY, 1.0f);
}
