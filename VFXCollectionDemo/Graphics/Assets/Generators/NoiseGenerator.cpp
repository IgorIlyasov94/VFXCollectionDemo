#include "NoiseGenerator.h"
#include "../../Common/Utilities.h"
#include "../../DirectX12Includes.h"

using namespace Common;
using namespace DirectX;

Graphics::Assets::Generators::NoiseGenerator::NoiseGenerator()
{
	std::random_device randomDevice;
	randomEngine = std::mt19937(randomDevice());
}

Graphics::Assets::Generators::NoiseGenerator::~NoiseGenerator()
{

}

void Graphics::Assets::Generators::NoiseGenerator::Generate(uint32_t width, uint32_t height, uint32_t depth,
	float3 scale, std::vector<float4>& textureData)
{
	Noise(width, height, depth, scale, textureData);

	for (uint32_t octaveIndex = 1u; octaveIndex < OCTAVES_NUMBER; octaveIndex++)
	{
		std::vector<float4> tNoise;
		Noise(width, height, depth, scale, tNoise);

		scale = float3(scale.x * 2.0f, scale.y * 2.0f, scale.z * 2.0f);

		std::vector<float4> nextOctaveNoise;
		Noise(width, height, depth, scale, nextOctaveNoise);

		LerpNoises(nextOctaveNoise, tNoise, textureData);
	}
}

void Graphics::Assets::Generators::NoiseGenerator::Noise(uint32_t width, uint32_t height, uint32_t depth,
	float3 scale, std::vector<float4>& textureData)
{
	uint32_t _width = std::max(width, 1u);
	uint32_t _height = std::max(height, 1u);
	uint32_t _depth = std::max(depth, 1u);

	auto mapSize = static_cast<uint32_t>(static_cast<uint64_t>(_width) * _height * _depth);

	auto randomGridSizeX = static_cast<uint32_t>(std::floor(RANDOM_GRID_SIZE_PER_128_PIXEL * _width * scale.x / BASE_MAP_SIZE));
	auto randomGridSizeY = static_cast<uint32_t>(std::floor(RANDOM_GRID_SIZE_PER_128_PIXEL * _height * scale.y / BASE_MAP_SIZE));
	auto randomGridSizeZ = static_cast<uint32_t>(std::floor(RANDOM_GRID_SIZE_PER_128_PIXEL * _depth * scale.z / BASE_MAP_SIZE));

	randomGridSizeX = std::max(randomGridSizeX, 1u);
	randomGridSizeY = std::max(randomGridSizeY, 1u);
	randomGridSizeZ = std::max(randomGridSizeZ, 1u);

	auto randomGridSize = static_cast<uint32_t>(static_cast<uint64_t>(randomGridSizeX) * randomGridSizeY * randomGridSizeZ);
	auto gridStepX = static_cast<uint32_t>(std::floor(_width / static_cast<float>(randomGridSizeX)));
	auto gridStepY = static_cast<uint32_t>(std::floor(_height / static_cast<float>(randomGridSizeY)));
	auto gridStepZ = static_cast<uint32_t>(std::floor(_depth / static_cast<float>(randomGridSizeZ)));

	textureData.resize(mapSize, float4(0.0f, 0.0f, 0.0f, 0.0f));

	std::vector<float4> randomGrid;
	PrepareGrid(randomGridSize, randomGrid);

	for (uint32_t texelIndex = 0u; texelIndex < mapSize; texelIndex++)
	{
		uint32_t xIndex{};
		uint32_t yIndex{};
		uint32_t zIndex{};
		GetXYZFromIndex(texelIndex, _width, _height, xIndex, yIndex, zIndex);

		uint32_t xGridIndex0{};
		uint32_t yGridIndex0{};
		uint32_t zGridIndex0{};

		if (_width > 1u)
			xGridIndex0 = static_cast<uint32_t>(std::floor((xIndex / static_cast<float>(_width)) * randomGridSizeX));
		xGridIndex0 = std::clamp(xGridIndex0, 0u, randomGridSizeX - 1);

		if (_height > 1u)
			yGridIndex0 = static_cast<uint32_t>(std::floor((yIndex / static_cast<float>(_height)) * randomGridSizeY));
		yGridIndex0 = std::clamp(yGridIndex0, 0u, randomGridSizeY - 1);

		if (_depth > 1u)
			zGridIndex0 = static_cast<uint32_t>(std::floor((zIndex / static_cast<float>(_depth)) * randomGridSizeZ));
		zGridIndex0 = std::clamp(zGridIndex0, 0u, randomGridSizeZ - 1);

		auto xGridIndex1 = (xGridIndex0 + 1u) >= randomGridSizeX ? 0u : xGridIndex0 + 1u;
		auto yGridIndex1 = (yGridIndex0 + 1u) >= randomGridSizeY ? 0u : yGridIndex0 + 1u;
		auto zGridIndex1 = (zGridIndex0 + 1u) >= randomGridSizeZ ? 0u : zGridIndex0 + 1u;

		auto gridIndexX0Y0Z0 = GetIndexFromXYZ(xGridIndex0, yGridIndex0, zGridIndex0, randomGridSizeX, randomGridSizeY);
		auto gridIndexX1Y0Z0 = GetIndexFromXYZ(xGridIndex1, yGridIndex0, zGridIndex0, randomGridSizeX, randomGridSizeY);
		auto gridIndexX0Y1Z0 = GetIndexFromXYZ(xGridIndex0, yGridIndex1, zGridIndex0, randomGridSizeX, randomGridSizeY);
		auto gridIndexX1Y1Z0 = GetIndexFromXYZ(xGridIndex1, yGridIndex1, zGridIndex0, randomGridSizeX, randomGridSizeY);
		auto gridIndexX0Y0Z1 = GetIndexFromXYZ(xGridIndex0, yGridIndex0, zGridIndex1, randomGridSizeX, randomGridSizeY);
		auto gridIndexX1Y0Z1 = GetIndexFromXYZ(xGridIndex1, yGridIndex0, zGridIndex1, randomGridSizeX, randomGridSizeY);
		auto gridIndexX0Y1Z1 = GetIndexFromXYZ(xGridIndex0, yGridIndex1, zGridIndex1, randomGridSizeX, randomGridSizeY);
		auto gridIndexX1Y1Z1 = GetIndexFromXYZ(xGridIndex1, yGridIndex1, zGridIndex1, randomGridSizeX, randomGridSizeY);

		auto tx = Curve((xIndex % gridStepX) / static_cast<float>(gridStepX));
		auto ty = Curve((yIndex % gridStepY) / static_cast<float>(gridStepY));
		auto tz = Curve((zIndex % gridStepZ) / static_cast<float>(gridStepZ));

		auto p = XMLoadFloat4(&randomGrid[gridIndexX0Y0Z0]);
		auto px = XMLoadFloat4(&randomGrid[gridIndexX1Y0Z0]);
		auto py = XMLoadFloat4(&randomGrid[gridIndexX0Y1Z0]);
		auto pxy = XMLoadFloat4(&randomGrid[gridIndexX1Y1Z0]);
		auto pz = XMLoadFloat4(&randomGrid[gridIndexX0Y0Z1]);
		auto pxz = XMLoadFloat4(&randomGrid[gridIndexX1Y0Z1]);
		auto pyz = XMLoadFloat4(&randomGrid[gridIndexX0Y1Z1]);
		auto pxyz = XMLoadFloat4(&randomGrid[gridIndexX1Y1Z1]);

		auto xBlend0 = XMVectorLerp(p, px, tx);
		auto xBlend1 = XMVectorLerp(py, pxy, tx);
		auto xBlend2 = XMVectorLerp(pz, pxz, tx);
		auto xBlend3 = XMVectorLerp(pyz, pxyz, tx);
		
		auto yBlend0 = XMVectorLerp(xBlend0, xBlend1, ty);
		auto yBlend1 = XMVectorLerp(xBlend2, xBlend3, ty);
		
		auto zBlend0 = XMVectorLerp(yBlend0, yBlend1, tz);

		XMStoreFloat4(&textureData[texelIndex], zBlend0);
	}
}

void Graphics::Assets::Generators::NoiseGenerator::PrepareGrid(uint32_t size, std::vector<float4>& grid)
{
	grid.resize(size, {});

	for (auto& node : grid)
		node = Utilities::Random4(randomEngine);
}

void Graphics::Assets::Generators::NoiseGenerator::LerpNoises(const std::vector<float4>& noise,
	const std::vector<float4>& tNoise, std::vector<float4>& targetNoise)
{
	for (uint32_t noiseIndex = 0u; noiseIndex < targetNoise.size(); noiseIndex++)
	{
		auto& noiseElement = noise[noiseIndex];
		auto& tNoiseElement = tNoise[noiseIndex];
		auto& targetNoiseElement = targetNoise[noiseIndex];

		targetNoiseElement.x = std::lerp(targetNoiseElement.x, noiseElement.x, tNoiseElement.x);
		targetNoiseElement.y = std::lerp(targetNoiseElement.y, noiseElement.y, tNoiseElement.y);
		targetNoiseElement.z = std::lerp(targetNoiseElement.z, noiseElement.z, tNoiseElement.z);
		targetNoiseElement.w = std::lerp(targetNoiseElement.w, noiseElement.w, tNoiseElement.w);
	}
}

float3 Graphics::Assets::Generators::NoiseGenerator::Rotate(float3 value, float3 axis, float angle)
{
	auto valueN = XMLoadFloat3(&value);
	auto axisN = XMLoadFloat3(&axis);
	axisN = XMVector3Normalize(axisN);
	auto quaternion = XMQuaternionRotationAxis(axisN, angle);

	valueN = XMVector3Rotate(valueN, quaternion);

	float3 result{};
	XMStoreFloat3(&result, valueN);

	return result;
}

void Graphics::Assets::Generators::NoiseGenerator::RotateIndices(uint32_t width, uint32_t height, uint32_t depth,
	uint32_t& x, uint32_t& y, uint32_t& z)
{
	auto rotatedIndices = float3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	rotatedIndices = Rotate(rotatedIndices, AXIS, ANGLE);

	x = static_cast<uint32_t>(std::round(rotatedIndices.x)) % width;
	y = static_cast<uint32_t>(std::round(rotatedIndices.y)) % height;
	z = static_cast<uint32_t>(std::round(rotatedIndices.z)) % depth;
}

float Graphics::Assets::Generators::NoiseGenerator::Curve(float x)
{
	return x * (x * (3.0f - x * 2.0f));
}

uint32_t Graphics::Assets::Generators::NoiseGenerator::GetIndexFromXYZ(uint32_t x, uint32_t y, uint32_t z,
	uint32_t width, uint32_t height)
{
	return static_cast<uint32_t>(width * (static_cast<uint64_t>(z) * height + y) + x);
}

void Graphics::Assets::Generators::NoiseGenerator::GetXYZFromIndex(uint32_t index, uint32_t width, uint32_t height,
	uint32_t& x, uint32_t& y, uint32_t& z)
{
	x = index % width;
	y = (index / width) % height;
	z = (index / width) / height;
}
