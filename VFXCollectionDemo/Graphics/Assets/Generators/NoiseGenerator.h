#pragma once

#include "../../DirectX12Includes.h"
#include "../../Resources/IResourceDesc.h"

namespace Graphics::Assets::Generators
{
	class NoiseGenerator
	{
	public:
		NoiseGenerator();
		~NoiseGenerator();

		void Generate(uint32_t width, uint32_t height, uint32_t depth, const float3& scale, std::vector<float4>& textureData);

	private:
		void GenerateWhiteNoiseMap(uint32_t width, uint32_t height, uint32_t depth, std::vector<floatN>& noiseMap);
		void GenerateWeights(int32_t halfSamplesNumber, std::vector<floatN>& weights);
		void GaussianBlur(int32_t width, int32_t height, int32_t depth, const float3& scale, std::vector<floatN>& noiseMap);

		void GaussianBlur(int32_t width, int32_t height, int32_t depth, int32_t halfSamplesNumber,
			int32_t directionMaskX, int32_t directionMaskY, int32_t directionMaskZ,
			const std::vector<floatN>& noiseMap, std::vector<floatN>& result);
		
		void Normalize(const std::vector<floatN>& noiseMap, std::vector<float4>& result);
		void Normalize(const floatN& value, const floatN& minValue, const floatN& diffValue, float4& result);
		void FindMinMax(const std::vector<floatN>& noiseMap, floatN& minValue, floatN& maxValue);

		float Sigma(float blurSize);
		float Weight(float x, float sigma);
		
		uint32_t GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height);
		void GetXYZFromIndex(uint32_t index, uint32_t width, uint32_t height, uint32_t& x, uint32_t& y, uint32_t& z);

		static constexpr uint32_t BASE_MAP_SIZE = 128u;
		static constexpr uint32_t BASE_GAUSSIAN_BLUR_SIZE = 2u;

		static constexpr float MIN_MAP_VALUE = 0.0f;
		static constexpr float MAX_MAP_VALUE = 1.0f;

		std::mt19937 randomEngine;
	};
}
