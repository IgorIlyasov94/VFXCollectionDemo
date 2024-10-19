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

		void Generate(uint32_t width, uint32_t height, uint32_t depth, float3 scale, std::vector<float4>& textureData);

	private:
		void Noise(uint32_t width, uint32_t height, uint32_t depth, float3 scale, std::vector<float4>& textureData);
		void PrepareGrid(uint32_t size, std::vector<float4>& grid);

		void LerpNoises(const std::vector<float4>& noise, const std::vector<float4>& tNoise,
			std::vector<float4>& targetNoise);

		float3 Rotate(float3 value, float3 axis, float angle);
		void RotateIndices(uint32_t width, uint32_t height, uint32_t depth, uint32_t& x, uint32_t& y, uint32_t& z);

		float Curve(float x);

		uint32_t GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height);
		void GetXYZFromIndex(uint32_t index, uint32_t width, uint32_t height, uint32_t& x, uint32_t& y, uint32_t& z);

		static constexpr uint32_t BASE_MAP_SIZE = 128u;
		static constexpr uint32_t RANDOM_GRID_SIZE_PER_128_PIXEL = 8u;

		static constexpr uint32_t OCTAVES_NUMBER = 3u;

		static constexpr float ANGLE = 1.05243353895198f;

		static constexpr float3 AXIS = float3(0.5773502691896257f, 0.5773502691896257f, 0.5773502691896257f);
		
		std::mt19937 randomEngine;
	};
}
