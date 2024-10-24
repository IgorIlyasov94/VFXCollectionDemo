#pragma once

#include "../../DirectX12Includes.h"
#include "../../Resources/IResourceDesc.h"

namespace Graphics::Assets::Generators
{
	class TurbulenceMapGenerator
	{
	public:
		TurbulenceMapGenerator();
		~TurbulenceMapGenerator();

		void Generate(uint32_t width, uint32_t height, uint32_t depth, const float3& scale,
			std::vector<floatN>& textureData);

	private:
		void Rotor(uint32_t width, uint32_t height, uint32_t depth,
			const std::vector<floatN>& map, std::vector<floatN>& result);

		float FindMaxRotorLength(const std::vector<floatN>& map);
		void FitRotorLength(float maxLength, std::vector<floatN>& map);

		void SmoothMap(uint32_t width, uint32_t height, uint32_t depth,
			std::vector<floatN>& map, std::vector<floatN>& result);

		floatN Rotor(const floatN& w_0, const floatN& w_x1, const floatN& w_y1, const floatN& w_z1);

		static constexpr float ROTOR_MULTIPLIER = 4.0f;

		static constexpr uint32_t BASE_MAP_SIZE = 128u;
		static constexpr uint32_t SMOOTH_FILTER_MAX_SIZE = 2;

		static constexpr int32_t BLUR_MAX_SIZE = 16;
	};
}
