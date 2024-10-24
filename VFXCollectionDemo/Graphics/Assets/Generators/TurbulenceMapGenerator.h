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
			const std::vector<floatN>& textureData, std::vector<floatN>& result);

		floatN Rotor(const floatN& w_0, const floatN& w_x1, const floatN& w_y1, const floatN& w_z1);

		static constexpr float ROTOR_SCALE = 10.0f;
		static constexpr int32_t BLUR_MAX_SIZE = 15;
	};
}
