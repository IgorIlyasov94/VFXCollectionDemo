#include "HLSLLoader.h"

D3D12_SHADER_BYTECODE Graphics::Assets::Loaders::HLSLLoader::Load(const std::filesystem::path& filePath, ShaderType type, ShaderVersion version)
{
	std::filesystem::path filePathCache(filePath);
	filePathCache.replace_extension(".hlslCACHE");
	auto loadCache = std::filesystem::exists(filePathCache);

	if (loadCache)
	{
		auto mainFileTimestamp = std::filesystem::last_write_time(filePath);
		auto cacheFileTimestamp = std::filesystem::last_write_time(filePathCache);

		loadCache = cacheFileTimestamp >= mainFileTimestamp;
	}

	if (loadCache)
		return LoadCache(filePathCache);

	CComPtr<IDxcUtils> dxcUtils;
	CComPtr<IDxcCompiler3> dxCompiler;
	HRESULT hrStatus;
	hrStatus = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	hrStatus = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxCompiler));

	std::wstring shaderProfile = GetShaderProfileString(type, version);
	const wchar_t* fileName = filePath.stem().c_str();

	std::vector<LPCWSTR> arguments;

#ifdef _DEBUG
	std::filesystem::path filePathPDB(filePath);
	filePathPDB.replace_extension(L".pdb");
	std::wstring filePathPDBString = L"-Fd " + std::filesystem::absolute(filePathPDB).generic_wstring();

	arguments.push_back(DXC_ARG_DEBUG);
	arguments.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
	arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
	arguments.push_back(filePathPDBString.c_str());
#else
	arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

	CComPtr<IDxcCompilerArgs> args;
	hrStatus = dxcUtils->BuildArguments(fileName, L"main", shaderProfile.c_str(), arguments.data(),
		static_cast<uint32_t>(arguments.size()), nullptr, 0u, &args);
	
	CComPtr<IDxcBlobEncoding> shaderText = nullptr;
	hrStatus = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderText);

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderText->GetBufferPointer();
	sourceBuffer.Size = shaderText->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP;

	CustomIncludeHandler customIncludeHandler(filePath, dxcUtils.p);

	CComPtr<IDxcResult> result;
	hrStatus = dxCompiler->Compile(&sourceBuffer, args->GetArguments(), args->GetCount(), &customIncludeHandler, IID_PPV_ARGS(&result));

	CComPtr<IDxcBlobUtf8> errorBuffer = nullptr;
	hrStatus = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBuffer), nullptr);

	result->GetStatus(&hrStatus);

	if (errorBuffer != nullptr && errorBuffer->GetStringLength() > 0)
	{
		auto bufferSize = errorBuffer->GetBufferSize();
		std::string errorMessage(errorBuffer->GetBufferSize(), ' ');

		if (bufferSize > 0)
		{
			auto addressStart = reinterpret_cast<uint8_t*>(errorBuffer->GetBufferPointer());
			auto addressEnd = reinterpret_cast<uint8_t*>(addressStart) + bufferSize;

			std::copy(addressStart, addressEnd, errorMessage.data());
			errorMessage = "HLSLLoader::CompileShader: Warnings and errors:\n\n" + errorMessage;
		}

		OutputDebugStringA(errorMessage.c_str());

		return {};
	}

	CComPtr<IDxcBlob> shaderBytecode;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBytecode), nullptr);

	auto bufferSize = shaderBytecode->GetBufferSize();
	auto addressStart = reinterpret_cast<const uint8_t*>(shaderBytecode->GetBufferPointer());
	auto addressEnd = reinterpret_cast<const uint8_t*>(addressStart) + bufferSize;

	auto bytecodeBuffer = new uint8_t[bufferSize];
	std::copy(addressStart, addressEnd, bytecodeBuffer);

	D3D12_SHADER_BYTECODE bytecode
	{
		bytecodeBuffer,
		bufferSize
	};

	SaveCache(filePathCache, bytecode);

#ifdef _DEBUG
	SavePDB(filePath, result.p);
#endif

	return bytecode;
}

std::wstring Graphics::Assets::Loaders::HLSLLoader::GetShaderProfileString(ShaderType type, ShaderVersion version)
{
	std::wstring profile;
	wchar_t versionNumber = L'0' + static_cast<int8_t>(version) - static_cast<int8_t>(ShaderVersion::SM_6_0);

	if (type == ShaderType::RAYTRACING_SHADER)
	{
		profile = L"lib_6_x";

		profile[6] = versionNumber;
	}
	else
	{
		profile = L"vs_6_0";

		if (type == ShaderType::HULL_SHADER)
			profile[0] = L'h';
		else if (type == ShaderType::DOMAIN_SHADER)
			profile[0] = L'd';
		else if (type == ShaderType::GEOMETRY_SHADER)
			profile[0] = L'g';
		else if (type == ShaderType::PIXEL_SHADER)
			profile[0] = L'p';
		else if (type == ShaderType::COMPUTE_SHADER)
			profile[0] = L'c';
		else if (type == ShaderType::MESH_SHADER)
			profile[0] = L'm';
		else if (type == ShaderType::AMPLIFICATION_SHADER)
			profile[0] = L'a';

		profile[5] = versionNumber;
	}

	return profile;
}

D3D12_SHADER_BYTECODE Graphics::Assets::Loaders::HLSLLoader::LoadCache(const std::filesystem::path& filePath)
{
	std::ifstream hlslFile(filePath, std::ios::binary);
	hlslFile.seekg(0, std::ios::end);

	auto bufferSize = static_cast<size_t>(hlslFile.tellg());

	hlslFile.seekg(0, std::ios::beg);

	uint8_t* bytecodeBuffer = new uint8_t[bufferSize];

	hlslFile.read(reinterpret_cast<char*>(bytecodeBuffer), bufferSize);

	D3D12_SHADER_BYTECODE bytecode
	{
		bytecodeBuffer,
		bufferSize
	};

	return bytecode;
}

void Graphics::Assets::Loaders::HLSLLoader::SaveCache(const std::filesystem::path& filePath,
	D3D12_SHADER_BYTECODE bytecode)
{
	std::ofstream hlslFile(filePath, std::ios::binary);
	hlslFile.write(reinterpret_cast<const char*>(bytecode.pShaderBytecode), bytecode.BytecodeLength);
}

void Graphics::Assets::Loaders::HLSLLoader::SavePDB(const std::filesystem::path& shaderPath, IDxcResult* result)
{
	CComPtr<IDxcBlob> debugData;
	CComPtr<IDxcBlobUtf16> debugDataPath;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&debugData), &debugDataPath);

	std::ofstream hlslFile(debugDataPath->GetStringPointer(), std::ios::binary);
	hlslFile.write(reinterpret_cast<const char*>(debugData->GetBufferPointer()), debugData->GetBufferSize());
}

Graphics::Assets::Loaders::HLSLLoader::CustomIncludeHandler::CustomIncludeHandler(const std::filesystem::path& filePath,
	IDxcUtils* utils)
	: _filePath(filePath), _utils(utils)
{

}

HRESULT STDMETHODCALLTYPE Graphics::Assets::Loaders::HLSLLoader::CustomIncludeHandler::LoadSource(_In_ LPCWSTR pFilename,
	_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource)
{
	CComPtr<IDxcBlobEncoding> pEncoding;
	std::filesystem::path path(pFilename);
	path = std::filesystem::weakly_canonical(path).make_preferred();
	path = std::filesystem::path(_filePath).remove_filename().string() + path.string();

	if (includedFiles.find(path) != includedFiles.end())
	{
		static const char nullStr[] = " ";
		_utils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, &pEncoding.p);
		*ppIncludeSource = pEncoding.Detach();
		return S_OK;
	}

	auto filename = path.wstring();

	HRESULT hr = _utils->LoadFile(filename.c_str(), nullptr, &pEncoding.p);
	if (SUCCEEDED(hr))
	{
		includedFiles.insert(path);
		*ppIncludeSource = pEncoding.Detach();
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE Graphics::Assets::Loaders::HLSLLoader::CustomIncludeHandler::QueryInterface(REFIID riid,
	_COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Graphics::Assets::Loaders::HLSLLoader::CustomIncludeHandler::AddRef(void)
{
	return 0;
}

ULONG STDMETHODCALLTYPE Graphics::Assets::Loaders::HLSLLoader::CustomIncludeHandler::Release(void)
{
	return 0;
}
