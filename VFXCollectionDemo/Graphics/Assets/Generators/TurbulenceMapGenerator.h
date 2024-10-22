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

		void Generate(uint32_t width, uint32_t height, uint32_t depth, const float3& scale, std::vector<float4>& textureData);

	private:
		void Rotor(uint32_t width, uint32_t height, uint32_t depth,
			const std::vector<float4>& textureData, std::vector<floatN>& result);
		
		floatN Rotor(const float4& w_0, const float4& w_x1, const float4& w_y1, const float4& w_z1);
	};
}
