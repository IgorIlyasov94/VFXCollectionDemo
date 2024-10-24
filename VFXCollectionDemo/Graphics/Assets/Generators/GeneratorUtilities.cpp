#include "GeneratorUtilities.h"

using namespace DirectX;

void Graphics::Assets::Generators::GeneratorUtilities::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	int32_t startSampleOffset, int32_t endSampleOffset, const float3& force,
	const std::vector<floatN>& map, std::vector<floatN>& result)
{
	std::vector<floatN> weights;
	GenerateWeights(startSampleOffset, endSampleOffset, weights);

	for (int32_t zIndex = 0u; zIndex < depth; zIndex++)
	{
		for (int32_t yIndex = 0u; yIndex < height; yIndex++)
		{
			for (int32_t xIndex = 0u; xIndex < width; xIndex++)
			{
				auto pixelIndex = GeneratorUtilities::GetIndexFromXYZ(xIndex, yIndex, zIndex, width, height);

				floatN sampleSum{};
				uint32_t weightIndex{};

				for (auto sampleOffset = startSampleOffset; sampleOffset <= endSampleOffset; sampleOffset++)
				{
					auto offsettedXIndex = static_cast<int32_t>(std::round(sampleOffset * force.x));
					auto offsettedYIndex = static_cast<int32_t>(std::round(sampleOffset * force.y));
					auto offsettedZIndex = static_cast<int32_t>(std::round(sampleOffset * force.z));
					offsettedXIndex = (width + xIndex + offsettedXIndex) % width;
					offsettedYIndex = (height + yIndex + offsettedYIndex) % height;
					offsettedZIndex = (depth + zIndex + offsettedZIndex) % depth;

					auto offsettedPixelIndex = GeneratorUtilities::GetIndexFromXYZ(offsettedXIndex, offsettedYIndex,
						offsettedZIndex, width, height);

					const auto& weight = weights[weightIndex++];

					sampleSum = XMVectorMultiplyAdd(map[offsettedPixelIndex], weight, sampleSum);
				}

				result[pixelIndex] = sampleSum;
			}
		}
	}
}

void Graphics::Assets::Generators::GeneratorUtilities::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	int32_t startSampleOffset, int32_t endSampleOffset, const std::vector<floatN>& forceMap,
	const std::vector<floatN>& map, std::vector<floatN>& result)
{
	std::vector<floatN> weights;
	GenerateWeights(startSampleOffset, endSampleOffset, weights);

	for (int32_t zIndex = 0u; zIndex < depth; zIndex++)
	{
		for (int32_t yIndex = 0u; yIndex < height; yIndex++)
		{
			for (int32_t xIndex = 0u; xIndex < width; xIndex++)
			{
				auto pixelIndex = GeneratorUtilities::GetIndexFromXYZ(xIndex, yIndex, zIndex, width, height);
				auto force = forceMap[pixelIndex];

				floatN sampleSum{};
				uint32_t weightIndex{};

				for (auto sampleOffset = startSampleOffset; sampleOffset <= endSampleOffset; sampleOffset++)
				{
					auto offsettedXIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[0]));
					auto offsettedYIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[1]));
					auto offsettedZIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[2]));
					offsettedXIndex = (width + xIndex + offsettedXIndex) % width;
					offsettedYIndex = (height + yIndex + offsettedYIndex) % height;
					offsettedZIndex = (depth + zIndex + offsettedZIndex) % depth;

					auto offsettedPixelIndex = GeneratorUtilities::GetIndexFromXYZ(offsettedXIndex, offsettedYIndex,
						offsettedZIndex, width, height);

					const auto& weight = weights[weightIndex++];

					sampleSum = XMVectorMultiplyAdd(map[offsettedPixelIndex], weight, sampleSum);
				}

				result[pixelIndex] = sampleSum;
			}
		}
	}
}

void Graphics::Assets::Generators::GeneratorUtilities::Normalize(const std::vector<floatN>& map, std::vector<floatN>& result)
{
	floatN minValue{};
	floatN maxValue{};
	FindMinMax(map, minValue, maxValue);

	auto diffValue = XMVectorSubtract(maxValue, minValue);

	for (auto pixelIndex = 0u; pixelIndex < map.size(); pixelIndex++)
		result[pixelIndex] = XMVectorDivide(XMVectorSubtract(map[pixelIndex], minValue), diffValue);
}

void Graphics::Assets::Generators::GeneratorUtilities::Normalize(std::vector<floatN>& map)
{
	floatN minValue{};
	floatN maxValue{};
	FindMinMax(map, minValue, maxValue);

	auto diffValue = XMVectorSubtract(maxValue, minValue);

	for (auto pixelIndex = 0u; pixelIndex < map.size(); pixelIndex++)
		map[pixelIndex] = XMVectorDivide(XMVectorSubtract(map[pixelIndex], minValue), diffValue);
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

float Graphics::Assets::Generators::GeneratorUtilities::Sigma(float maxAbsX)
{
	auto sigma = maxAbsX - 2.0f;
	sigma /= 8.0f;
	sigma *= 3.35f;
	sigma += 0.65f;

	return sigma;
}

float Graphics::Assets::Generators::GeneratorUtilities::NormalDistribution(float x, float sigma)
{
	auto distribution = -x * x / (2.0f * sigma * sigma);
	distribution = std::exp(distribution);
	distribution /= static_cast<float>(sigma * std::sqrt(2.0 * std::numbers::pi));

	return distribution;
}

void Graphics::Assets::Generators::GeneratorUtilities::GenerateWeights(int32_t startSampleOffset, int32_t endSampleOffset,
	std::vector<floatN>& weights)
{
	auto samplesNumber = endSampleOffset - startSampleOffset + 1;
	auto sigma = Sigma(static_cast<float>(std::round(samplesNumber * 0.5f)));
	
	weights.resize(samplesNumber);

	auto weightIndex = 0u;

	for (auto sampleIndex = startSampleOffset; sampleIndex <= endSampleOffset; sampleIndex++)
	{
		weights[weightIndex].m128_f32[0] = NormalDistribution(static_cast<float>(sampleIndex), sigma);
		weights[weightIndex].m128_f32[1] = weights[weightIndex].m128_f32[0];
		weights[weightIndex].m128_f32[2] = weights[weightIndex].m128_f32[0];
		weights[weightIndex].m128_f32[3] = weights[weightIndex].m128_f32[0];

		weightIndex++;
	}
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
