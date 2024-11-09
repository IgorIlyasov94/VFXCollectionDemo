#pragma once

#include "../../DirectX12Includes.h"
#include "../../Resources/IResourceDesc.h"

namespace Graphics::Assets::Generators
{
	class GeneratorUtilities
	{
	public:
		static void GaussianBlur(int32_t width, int32_t height, int32_t depth, int32_t startSampleOffset, int32_t endSampleOffset,
			const float3& force, const std::vector<floatN>& map, std::vector<floatN>& result);

		static void GaussianBlur(int32_t width, int32_t height, int32_t depth, int32_t startSampleOffset, int32_t endSampleOffset,
			const std::vector<floatN>& forceMap, const std::vector<floatN>& map, std::vector<floatN>& result);

		static void Normalize(const std::vector<floatN>& map, std::vector<floatN>& result);
		static void Normalize(std::vector<floatN>& map);

		static uint32_t GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height);
		static void GetXYZFromIndex(uint32_t index, const uint3& size, uint3& xyzIndex);

		static float Sigma(float maxAbsX);
		static float NormalDistribution(float x, float sigma);

	private:
		GeneratorUtilities() = delete;
		GeneratorUtilities(const GeneratorUtilities&) = delete;
		GeneratorUtilities(GeneratorUtilities&&) = delete;
		GeneratorUtilities operator=(const GeneratorUtilities&) = delete;
		GeneratorUtilities operator=(GeneratorUtilities&&) = delete;

		static void GenerateWeights(int32_t startSampleOffset, int32_t endSampleOffset, std::vector<floatN>& weights);

		static void FindMinMax(const std::vector<floatN>& map, floatN& minValue, floatN& maxValue);
		static floatN DirectionalBlur(int32_t startSampleOffset, int32_t endSampleOffset, const std::vector<floatN>& weights,
			const std::vector<floatN>& map, const floatN& force, const uint3& index, const uint3& size);

		static constexpr uint32_t MIN_BLOCKS_PER_THREAD = 65535u;
		static constexpr uint32_t THREAD_THRESHOLD = MIN_BLOCKS_PER_THREAD * 2u;
	};
}
