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
		static D3D12_SHADER_BYTECODE Load(const std::filesystem::path& filePath, ShaderType type,
			ShaderVersion version, const std::vector<DxcDefine>& defines = {});

	private:
		HLSLLoader() = delete;
		~HLSLLoader() = delete;
		HLSLLoader(const HLSLLoader&) = delete;
		HLSLLoader(HLSLLoader&&) = delete;
		HLSLLoader& operator=(const HLSLLoader&) = delete;
		HLSLLoader& operator=(HLSLLoader&&) = delete;

		static std::wstring GetShaderProfileString(ShaderType type, ShaderVersion version);
		static uint64_t GetHashFromDefines(const std::vector<DxcDefine>& defines);

		static D3D12_SHADER_BYTECODE LoadCache(const std::filesystem::path& filePath);
		static void SaveCache(const std::filesystem::path& filePath, D3D12_SHADER_BYTECODE bytecode);

		static void SavePDB(const std::filesystem::path& shaderPath, IDxcResult* result);

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

		template <char c0, char c1, char c2, char c3>
		static constexpr uint32_t DXILFourCC()
		{
			return static_cast<uint32_t>(c0) |
				static_cast<uint32_t>(c1) << 8 |
				static_cast<uint32_t>(c2) << 16 |
				static_cast<uint32_t>(c3) << 24;
		}

		enum DxilFourCC
		{
			DFCC_Container = DXILFourCC<'D', 'X', 'B', 'C'>(),
			DFCC_ResourceDef = DXILFourCC<'R', 'D', 'E', 'F'>(),
			DFCC_InputSignature = DXILFourCC<'I', 'S', 'G', '1'>(),
			DFCC_OutputSignature = DXILFourCC<'O', 'S', 'G', '1'>(),
			DFCC_PatchConstantSignature = DXILFourCC<'P', 'S', 'G', '1'>(),
			DFCC_ShaderStatistics = DXILFourCC<'S', 'T', 'A', 'T'>(),
			DFCC_ShaderDebugInfoDXIL = DXILFourCC<'I', 'L', 'D', 'B'>(),
			DFCC_ShaderDebugName = DXILFourCC<'I', 'L', 'D', 'N'>(),
			DFCC_FeatureInfo = DXILFourCC<'S', 'F', 'I', '0'>(),
			DFCC_PrivateData = DXILFourCC<'P', 'R', 'I', 'V'>(),
			DFCC_RootSignature = DXILFourCC<'R', 'T', 'S', '0'>(),
			DFCC_DXIL = DXILFourCC<'D', 'X', 'I', 'L'>(),
			DFCC_PipelineStateValidation = DXILFourCC<'P', 'S', 'V', '0'>(),
			DFCC_RuntimeData = DXILFourCC<'R', 'D', 'A', 'T'>(),
			DFCC_ShaderHash = DXILFourCC<'H', 'A', 'S', 'H'>()
		};

		struct DxilShaderDebugName
		{
			uint16_t Flags;
			uint16_t NameLength;
		};
	};
}
