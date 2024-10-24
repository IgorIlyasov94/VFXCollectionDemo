#include "NoiseGenerator.h"
#include "../../Common/Utilities.h"
#include "../../DirectX12Includes.h"
#include "GeneratorUtilities.h"

using namespace Common;
using namespace DirectX;

Graphics::Assets::Generators::NoiseGenerator::NoiseGenerator()
{
	std::random_device randomDevice;
	randomEngine = std::mt19937(randomDevice());
}

Graphics::Assets::Generators::NoiseGenerator::~NoiseGenerator()
{

}

void Graphics::Assets::Generators::NoiseGenerator::Generate(uint32_t width, uint32_t height, uint32_t depth,
	const float3& scale, std::vector<floatN>& textureData)
{
	auto mapSize = static_cast<uint32_t>(static_cast<uint64_t>(width) * height * depth);
	textureData.resize(mapSize);

	GenerateWhiteNoiseMap(width, height, depth, textureData);
	GaussianBlur(static_cast<int32_t>(width), static_cast<int32_t>(height), static_cast<int32_t>(depth), scale, textureData);
}

void Graphics::Assets::Generators::NoiseGenerator::GenerateWhiteNoiseMap(uint32_t width, uint32_t height, uint32_t depth,
	std::vector<floatN>& noiseMap)
{
	uint32_t _width = std::max(width, 1u);
	uint32_t _height = std::max(height, 1u);
	uint32_t _depth = std::max(depth, 1u);

	for (auto& element : noiseMap)
	{
		auto temp = Utilities::Random4(randomEngine);
		element = XMLoadFloat4(&temp);
	}
}

void Graphics::Assets::Generators::NoiseGenerator::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	const float3& scale, std::vector<floatN>& noiseMap)
{
	auto halfSamplesNumberX = static_cast<int32_t>(std::round(scale.x * BASE_GAUSSIAN_BLUR_SIZE * width / BASE_MAP_SIZE));
	auto halfSamplesNumberY = static_cast<int32_t>(std::round(scale.y * BASE_GAUSSIAN_BLUR_SIZE * height / BASE_MAP_SIZE));
	auto halfSamplesNumberZ = static_cast<int32_t>(std::round(scale.z * BASE_GAUSSIAN_BLUR_SIZE * depth / BASE_MAP_SIZE));

	halfSamplesNumberX = std::max(halfSamplesNumberX, 1);
	halfSamplesNumberY = std::max(halfSamplesNumberY, 1);
	halfSamplesNumberZ = std::max(halfSamplesNumberZ, 1);

	std::vector<floatN> temp;
	temp.resize(noiseMap.size());
	
	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberX, halfSamplesNumberX,
		float3(1.0f, 0.0f, 0.0f), noiseMap, temp);

	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberY, halfSamplesNumberY,
		float3(0.0f, 1.0f, 0.0f), temp, noiseMap);

	GeneratorUtilities::GaussianBlur(width, height, depth, -halfSamplesNumberZ, halfSamplesNumberZ,
		float3(0.0f, 0.0f, 1.0f), noiseMap, temp);

	GeneratorUtilities::Normalize(temp, noiseMap);
}
