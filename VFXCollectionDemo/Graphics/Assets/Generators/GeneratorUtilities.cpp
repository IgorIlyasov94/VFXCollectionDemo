#include "GeneratorUtilities.h"

using namespace DirectX;

void Graphics::Assets::Generators::GeneratorUtilities::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	int32_t startSampleOffset, int32_t endSampleOffset, const float3& force,
	const std::vector<floatN>& map, std::vector<floatN>& result)
{
	std::vector<floatN> weights;
	GenerateWeights(startSampleOffset, endSampleOffset, weights);

	auto forceV = XMLoadFloat3(&force);
	auto size = uint3(width, height, depth);

	auto threadFunc = [&map, &result, startSampleOffset, endSampleOffset,
		&forceV, &size, &weights](uint32_t startIndex, uint32_t endIndex)
		{
			for (uint32_t index = startIndex; index < endIndex; index++)
			{
				uint3 xyzIndex{};
				GeneratorUtilities::GetXYZFromIndex(index, size, xyzIndex);

				result[index] = DirectionalBlur(startSampleOffset, endSampleOffset, weights, map, forceV, xyzIndex, size);
			}
		};

	if (map.size() > THREAD_THRESHOLD)
	{
		const uint32_t maxThreads = std::thread::hardware_concurrency();
		const uint32_t numThreads = std::min(static_cast<uint32_t>(map.size() / MIN_BLOCKS_PER_THREAD), maxThreads);
		const uint32_t elementsPerThread = static_cast<uint32_t>(map.size() / numThreads);
		
		std::vector<std::thread> threads;
		threads.reserve(numThreads);
		
		for (uint32_t threadIndex = 0u; threadIndex < numThreads; threadIndex++)
		{
			auto startIndex = static_cast<uint32_t>(static_cast<uint64_t>(threadIndex) * elementsPerThread);
			uint32_t endIndex{};
			
			if (threadIndex == (numThreads - 1))
				endIndex = static_cast<uint32_t>(map.size());
			else
				endIndex = static_cast<uint32_t>(static_cast<uint64_t>(threadIndex + 1u) * elementsPerThread);
			
			threads.push_back(std::thread(threadFunc, startIndex, endIndex));
		}
		
		for (uint32_t threadIndex = 0u; threadIndex < numThreads; threadIndex++)
			if (threads[threadIndex].joinable())
				threads[threadIndex].join();
	}
	else
		threadFunc(0u, static_cast<uint32_t>(map.size()));
}

void Graphics::Assets::Generators::GeneratorUtilities::GaussianBlur(int32_t width, int32_t height, int32_t depth,
	int32_t startSampleOffset, int32_t endSampleOffset, const std::vector<floatN>& forceMap,
	const std::vector<floatN>& map, std::vector<floatN>& result)
{
	std::vector<floatN> weights;
	GenerateWeights(startSampleOffset, endSampleOffset, weights);

	auto size = uint3(width, height, depth);

	for (uint32_t index = 0u; index < map.size(); index++)
	{
		uint3 xyzIndex{};
		GeneratorUtilities::GetXYZFromIndex(index, size, xyzIndex);
		auto& force = forceMap[index];

		result[index] = DirectionalBlur(startSampleOffset, endSampleOffset, weights, map, force, xyzIndex, size);
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

void Graphics::Assets::Generators::GeneratorUtilities::GetXYZFromIndex(uint32_t index, const uint3& size, uint3& xyzIndex)
{
	xyzIndex.x = index % size.x;
	xyzIndex.y = (index / size.x) % size.y;
	xyzIndex.z = (index / size.x) / size.y;
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

floatN Graphics::Assets::Generators::GeneratorUtilities::DirectionalBlur(int32_t startSampleOffset, int32_t endSampleOffset,
	const std::vector<floatN>& weights, const std::vector<floatN>& map, const floatN& force, const uint3& index, const uint3& size)
{
	floatN sampleSum{};
	uint32_t weightIndex{};

	for (auto sampleOffset = startSampleOffset; sampleOffset <= endSampleOffset; sampleOffset++)
	{
		auto offsettedXIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[0]));
		auto offsettedYIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[1]));
		auto offsettedZIndex = static_cast<int32_t>(std::round(sampleOffset * force.m128_f32[2]));
		offsettedXIndex = (size.x + index.x + offsettedXIndex) % size.x;
		offsettedYIndex = (size.y + index.y + offsettedYIndex) % size.y;
		offsettedZIndex = (size.z + index.z + offsettedZIndex) % size.z;

		auto offsettedPixelIndex = GeneratorUtilities::GetIndexFromXYZ(offsettedXIndex, offsettedYIndex,
			offsettedZIndex, size.x, size.y);

		const auto& weight = weights[weightIndex++];

		sampleSum = XMVectorMultiplyAdd(map[offsettedPixelIndex], weight, sampleSum);
	}

	return sampleSum;
}
