#pragma once

#include "../../DirectX12Includes.h"
#include "../../Resources/IResourceDesc.h"

namespace Graphics::Assets::Generators
{
	class GeneratorUtilities
	{
	public:
		static void Normalize(const std::vector<floatN>& map, std::vector<float4>& result);

		static uint32_t GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height);
		static void GetXYZFromIndex(uint32_t index, uint32_t width, uint32_t height, uint32_t& x, uint32_t& y, uint32_t& z);

	private:
		GeneratorUtilities() = delete;
		GeneratorUtilities(const GeneratorUtilities&) = delete;
		GeneratorUtilities(GeneratorUtilities&&) = delete;
		GeneratorUtilities operator=(const GeneratorUtilities&) = delete;
		GeneratorUtilities operator=(GeneratorUtilities&&) = delete;

		static void Normalize(const floatN& value, const floatN& minValue, const floatN& diffValue, float4& result);
		static void FindMinMax(const std::vector<floatN>& map, floatN& minValue, floatN& maxValue);
	};
}
