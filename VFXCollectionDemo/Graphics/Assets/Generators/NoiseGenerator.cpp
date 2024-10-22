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
	const float3& scale, std::vector<float4>& textureData)
{
	std::vector<floatN> tempData;
	GenerateWhiteNoiseMap(width, height, depth, tempData);
	GaussianBlur(static_cast<int32_t>(width), static_cast<int32_t>(height), static_cast<int32_t>(depth), scale, tempData);
	GeneratorUtilities::Normalize(tempData, textureData);
}

void Graphics::Assets::Generators::NoiseGenerator::GenerateWhiteNoiseMap(uint32_t width, uint32_t height, uint32_t depth,
	std::vector<floatN>& noiseMap)
{
	uint32_t _width = std::max(width, 1u);
	uint32_t _height = std::max(height, 1u);
	uint32_t _depth = std::max(depth, 1u);

	auto mapSize = static_cast<uint32_t>(static_cast<uint64_t>(_width) * _height * _depth);
	noiseMap.resize(mapSize, {});

	for (auto& element : noiseMap)
	{
		auto temp = Utilities::Random4(randomEngine);
		element = XMLoadFloat4(&temp);
	}
}

void Graphics::Assets::Generators::NoiseGenerator::GenerateWeights(int32_t halfSamplesNumber,
	std::vector<floatN>& weights)
{
	auto sigma = Sigma(static_cast<float>(halfSamplesNumber));
	auto samplesNumber = halfSamplesNumber * 2 + 1;

	weights.resize(samplesNumber);

	for (auto sampleIndex = -halfSamplesNumber; sampleIndex <= halfSamplesNumber; sampleIndex++)
	{
		auto weightIndex = sampleIndex + halfSamplesNumber;
		weights[weightIndex].m128_f32[0] = Weight(static_cast<float>(sampleIndex), sigma);
		weights[weightIndex].m128_f32[1] = weights[weightIndex].m128_f32[0];
		weights[weightIndex].m128_f32[2] = weights[weightIndex].m128_f32[0];
		weights[weightIndex].m128_f32[3] = weights[weightIndex].m128_f32[0];
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

	GaussianBlur(width, height, depth, halfSamplesNumberX, 1, 0, 0, noiseMap, temp);
	GaussianBlur(width, height, depth, halfSamplesNumberY, 0, 1, 0, temp, noiseMap);
	GaussianBlur(width, height, depth, halfSamplesNumberZ, 0, 0, 1, noiseMap, temp);

	noiseMap = temp;
}

void Graphics::Assets::Generators::NoiseGenerator::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	int32_t halfSamplesNumber, int32_t directionMaskX, int32_t directionMaskY, int32_t directionMaskZ,
	const std::vector<floatN>& noiseMap, std::vector<floatN>& result)
{
	std::vector<floatN> weights;
	GenerateWeights(halfSamplesNumber, weights);

	for (int32_t zIndex = 0u; zIndex < depth; zIndex++)
	{
		for (int32_t yIndex = 0u; yIndex < height; yIndex++)
		{
			for (int32_t xIndex = 0u; xIndex < width; xIndex++)
			{
				auto pixelIndex = GeneratorUtilities::GetIndexFromXYZ(xIndex, yIndex, zIndex, width, height);
				
				floatN sampleSum{};

				for (auto sampleOffset = -halfSamplesNumber; sampleOffset <= halfSamplesNumber; sampleOffset++)
				{
					auto offsettedXIndex = (width + xIndex + sampleOffset * directionMaskX) % width;
					auto offsettedYIndex = (height + yIndex + sampleOffset * directionMaskY) % height;
					auto offsettedZIndex = (depth + zIndex + sampleOffset * directionMaskZ) % depth;
					auto offsettedPixelIndex = GeneratorUtilities::GetIndexFromXYZ(offsettedXIndex, offsettedYIndex,
						offsettedZIndex, width, height);

					auto weightIndex = static_cast<uint32_t>(sampleOffset + halfSamplesNumber);
					const auto& weight = weights[weightIndex];

					sampleSum = XMVectorMultiplyAdd(noiseMap[offsettedPixelIndex], weight, sampleSum);
				}

				result[pixelIndex] = sampleSum;
			}
		}
	}
}

float Graphics::Assets::Generators::NoiseGenerator::Sigma(float blurSize)
{
	auto sigma = blurSize - 2.0f;
	sigma /= 8.0f;
	sigma *= 3.35f;
	sigma += 0.65f;

	return sigma;
}

float Graphics::Assets::Generators::NoiseGenerator::Weight(float x, float sigma)
{
	auto weight = -x * x / (2.0f * sigma * sigma);
	weight = std::exp(weight);
	weight /= static_cast<float>(sigma * std::sqrt(2.0 * std::numbers::pi));

	return weight;
}
