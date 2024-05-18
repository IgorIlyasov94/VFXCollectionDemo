#pragma once

#include "../Includes.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <atlbase.h>
#include <d3d12shader.h>

#include <DirectXMath.h>

using floatN = DirectX::XMVECTOR;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using float4x4 = DirectX::XMMATRIX;
