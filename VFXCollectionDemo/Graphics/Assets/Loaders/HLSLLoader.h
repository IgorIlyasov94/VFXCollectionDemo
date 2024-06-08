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
		HULL_SHADER = 2u,
		DOMAIN_SHADER = 3u,
		GEOMETRY_SHADER = 4u,
		MESH_SHADER = 5u,
		AMPLIFICATION_SHADER = 6u,
		PIXEL_SHADER = 7u,
		RAYTRACING_SHADER = 8u,
		COMPUTE_SHADER = 9u
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

		static D3D12_SHADER_BYTECODE LoadCache(const std::filesystem::path& filePath);
		static void SaveCache(const std::filesystem::path& filePath, D3D12_SHADER_BYTECODE bytecode);

		class CustomIncludeHandler : public IDxcIncludeHandler
		{
		public:
			CustomIncludeHandler(const std::filesystem::path& filePath, IDxcUtils* utils);

			HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename,
				_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override;

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
				_COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;

			ULONG STDMETHODCALLTYPE AddRef(void) override;
			ULONG STDMETHODCALLTYPE Release(void) override;

			IDxcUtils* _utils;
			const std::filesystem::path& _filePath;
			std::unordered_set<std::filesystem::path> includedFiles;
		};
	};
}
