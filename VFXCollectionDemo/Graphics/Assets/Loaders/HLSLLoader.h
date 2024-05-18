#pragma once

#include "../../DirectX12Includes.h"

namespace Graphics::Assets::Loaders
{
	enum class ShaderVersion : uint32_t
	{
		SM_6_0 = 0u,
		SM_6_1 = 1u,
		SM_6_2 = 2u,
		SM_6_3 = 3u,
		SM_6_4 = 4u,
		SM_6_5 = 5u,
		SM_6_6 = 6u
	};

	enum class ShaderType : uint32_t
	{
		UNDEFINED = 0u,
		VERTEX_SHADER = 1u,
		HULL_SHADER = 1u << 1u,
		DOMAIN_SHADER = 1u << 2u,
		GEOMETRY_SHADER = 1u << 3u,
		MESH_SHADER = 1u << 4u,
		AMPLIFICATION_SHADER = 1u << 5u,
		PIXEL_SHADER = 1u << 6u,
		RAYTRACING_SHADER = 1u << 7u,
		COMPUTE_SHADER = 1u << 8u
	};

	class HLSLLoader final
	{
	public:
		static D3D12_SHADER_BYTECODE Load(const std::filesystem::path& filePath, ShaderType type, ShaderVersion version);

	private:
		HLSLLoader() = delete;
		~HLSLLoader() = delete;
		HLSLLoader(const HLSLLoader&) = delete;
		HLSLLoader(HLSLLoader&&) = delete;
		HLSLLoader& operator=(const HLSLLoader&) = delete;
		HLSLLoader& operator=(HLSLLoader&&) = delete;

		static std::wstring GetShaderProfileString(ShaderType type, ShaderVersion version);
	};
}
