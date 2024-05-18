#include "HLSLLoader.h"

D3D12_SHADER_BYTECODE Graphics::Assets::Loaders::HLSLLoader::Load(const std::filesystem::path& filePath, ShaderType type, ShaderVersion version)
{
	CComPtr<IDxcUtils> dxcUtils;
	CComPtr<IDxcCompiler3> dxCompiler;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxCompiler));

	CComPtr<IDxcIncludeHandler> pIncludeHandler;
	dxcUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

	std::wstring shaderProfile = GetShaderProfileString(type, version);
	const wchar_t* fileName = filePath.stem().c_str();

	CComPtr<IDxcCompilerArgs> args;
	dxcUtils->BuildArguments(fileName, L"main", shaderProfile.c_str(), nullptr, 0u, nullptr, 0u, &args);

	CComPtr<IDxcBlobEncoding> shaderText = nullptr;
	dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderText);

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderText->GetBufferPointer();
	sourceBuffer.Size = shaderText->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP;

	CComPtr<IDxcResult> result;
	dxCompiler->Compile(&sourceBuffer, args->GetArguments(), args->GetCount(), nullptr, IID_PPV_ARGS(&result));

	CComPtr<IDxcBlobUtf16> errorBuffer = nullptr;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBuffer), nullptr);

	HRESULT hrStatus;
	result->GetStatus(&hrStatus);

	if (errorBuffer != nullptr && errorBuffer->GetStringLength() > 0 || FAILED(hrStatus))
	{
		auto bufferSize = errorBuffer->GetBufferSize();
		std::wstring errorMessage(errorBuffer->GetBufferSize(), L' ');

		if (bufferSize > 0)
		{
			auto addressStart = reinterpret_cast<uint8_t*>(errorBuffer->GetBufferPointer());
			auto addressEnd = reinterpret_cast<uint8_t*>(addressStart) + bufferSize;

			std::copy(addressStart, addressEnd, errorMessage.data());
			errorMessage = L"HLSLLoader::CompileShader: Warnings and errors:\n\n" + errorMessage;
		}

		OutputDebugString(errorMessage.c_str());

		return {};
	}

	CComPtr<IDxcBlob> shaderBytecode = nullptr;
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
