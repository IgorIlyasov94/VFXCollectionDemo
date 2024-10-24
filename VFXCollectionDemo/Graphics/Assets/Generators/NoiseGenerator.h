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

		void Generate(uint32_t width, uint32_t height, uint32_t depth, const float3& scale, std::vector<floatN>& textureData);

	private:
		void GenerateWhiteNoiseMap(uint32_t width, uint32_t height, uint32_t depth, std::vector<floatN>& noiseMap);
		void GaussianBlur(int32_t width, int32_t height, int32_t depth, const float3& scale, std::vector<floatN>& noiseMap);

		static constexpr uint32_t BASE_MAP_SIZE = 128u;
		static constexpr uint32_t BASE_GAUSSIAN_BLUR_SIZE = 2u;

		static constexpr float MIN_MAP_VALUE = 0.0f;
		static constexpr float MAX_MAP_VALUE = 1.0f;

		std::mt19937 randomEngine;
	};
}
