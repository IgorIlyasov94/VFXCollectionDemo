#pragma once

#include "../Includes.h"

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <atlbase.h>
#include <d3d12shader.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

using ubyte4 = DirectX::PackedVector::XMUBYTE4;
using ubyte2 = DirectX::PackedVector::XMUBYTE2;

using floatN = DirectX::XMVECTOR;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using float4x4 = DirectX::XMMATRIX;
