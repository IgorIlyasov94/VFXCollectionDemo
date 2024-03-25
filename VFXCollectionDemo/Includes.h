#pragma once

#include <vector>
#include <queue>
#include <sstream>
#include <exception>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>

#include <wrl/client.h>

using namespace Microsoft::WRL;
using namespace DirectX;

using floatN = XMVECTOR;
using float4 = XMFLOAT4;
using float3 = XMFLOAT3;
using float2 = XMFLOAT2;
using float4x4 = XMMATRIX;

inline void ThrowIfFailed(HRESULT hResult, const char* errorMessage)
{
    if (FAILED(hResult))
    {
        std::stringstream fullMessage;
        fullMessage << errorMessage << "\nHRESULT: " << "0x" << std::hex << hResult;

        throw std::runtime_error(fullMessage.str());
    }
}
