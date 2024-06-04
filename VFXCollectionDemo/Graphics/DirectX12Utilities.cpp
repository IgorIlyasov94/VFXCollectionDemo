#include "DirectX12Utilities.h"

void Graphics::DirectX12Utilities::WaitForGPU(ID3D12CommandQueue* queue, ID3D12Fence* fence, HANDLE& fenceEvent, uint64_t& fenceValue)
{
	queue->Signal(fence, fenceValue);
	fence->SetEventOnCompletion(fenceValue, fenceEvent);

	WaitForSingleObjectEx(fenceEvent, INFINITE, false);

	fenceValue++;
}

D3D12_SHADER_RESOURCE_VIEW_DESC Graphics::DirectX12Utilities::CreateSRVDesc(const Resources::TextureDesc& desc)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = desc.format;
	shaderResourceViewDesc.ViewDimension = desc.srvDimension;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		shaderResourceViewDesc.Texture1D.MipLevels = desc.mipLevels;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		shaderResourceViewDesc.Texture1DArray.MipLevels = desc.mipLevels;
		shaderResourceViewDesc.Texture1DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
		shaderResourceViewDesc.Texture2D.MipLevels = desc.mipLevels;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		shaderResourceViewDesc.Texture2DArray.MipLevels = desc.mipLevels;
		shaderResourceViewDesc.Texture2DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURECUBE)
		shaderResourceViewDesc.TextureCube.MipLevels = desc.mipLevels;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
	{
		shaderResourceViewDesc.TextureCubeArray.MipLevels = desc.mipLevels;
		shaderResourceViewDesc.TextureCubeArray.NumCubes = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
		shaderResourceViewDesc.Texture3D.MipLevels = desc.mipLevels;

	return shaderResourceViewDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC Graphics::DirectX12Utilities::CreateSRVDesc(const Resources::BufferDesc& desc)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = desc.dataStride == 0 ? desc.format : (desc.dataStride == 1) ?
		DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
	shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shaderResourceViewDesc.Buffer.Flags = (desc.dataStride == 1) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
	shaderResourceViewDesc.Buffer.NumElements = desc.numElements;
	shaderResourceViewDesc.Buffer.StructureByteStride = (desc.dataStride > 1) ? desc.dataStride : 0u;

	return shaderResourceViewDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC Graphics::DirectX12Utilities::CreateUAVDesc(const Resources::TextureDesc& desc)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
	unorderedAccessViewDesc.Format = desc.format;

	if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		unorderedAccessViewDesc.Texture1DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		unorderedAccessViewDesc.Texture2DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
	{
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		unorderedAccessViewDesc.Texture3D.WSize = desc.depth;
	}

	return unorderedAccessViewDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC Graphics::DirectX12Utilities::CreateUAVDesc(const Resources::BufferDesc& desc)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
	unorderedAccessViewDesc.Format = desc.dataStride == 0 ? desc.format : (desc.dataStride == 1) ?
		DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
	unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	unorderedAccessViewDesc.Buffer.Flags = desc.dataStride == 1 ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
	unorderedAccessViewDesc.Buffer.NumElements = desc.numElements;
	unorderedAccessViewDesc.Buffer.StructureByteStride = (desc.dataStride > 1) ? desc.dataStride : 0u;

	return unorderedAccessViewDesc;
}

D3D12_RENDER_TARGET_VIEW_DESC Graphics::DirectX12Utilities::CreateRTVDesc(const Resources::TextureDesc& desc)
{
	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
	renderTargetViewDesc.Format = desc.format;

	if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		renderTargetViewDesc.Texture1DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		renderTargetViewDesc.Texture2DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
	{
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		renderTargetViewDesc.Texture3D.WSize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DMS)
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
	{
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		renderTargetViewDesc.Texture2DMSArray.ArraySize = desc.depth;
	}

	return renderTargetViewDesc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC Graphics::DirectX12Utilities::CreateDSVDesc(const Resources::TextureDesc& desc)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = desc.depthBit == 32u ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
	
	if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
	{
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		depthStencilViewDesc.Texture1DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
	{
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		depthStencilViewDesc.Texture2DArray.ArraySize = desc.depth;
	}
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DMS)
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	else if (desc.srvDimension == D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
	{
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		depthStencilViewDesc.Texture2DMSArray.ArraySize = desc.depth;
	}

	return depthStencilViewDesc;
}

D3D12_RASTERIZER_DESC Graphics::DirectX12Utilities::CreateRasterizeDesc(D3D12_CULL_MODE mode)
{
	D3D12_RASTERIZER_DESC desc{};
	desc.AntialiasedLineEnable = false;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	desc.CullMode = mode;
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.DepthClipEnable = true;
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.ForcedSampleCount = 0;
	desc.FrontCounterClockwise = false;
	desc.MultisampleEnable = false;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

	return desc;
}

D3D12_DEPTH_STENCIL_DESC Graphics::DirectX12Utilities::CreateDepthStencilDesc(bool enableZTest)
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = enableZTest;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.StencilEnable = false;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	const D3D12_DEPTH_STENCILOP_DESC depthStencilOpDesc =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
	};

	desc.BackFace = depthStencilOpDesc;
	desc.FrontFace = depthStencilOpDesc;

	return desc;
}

D3D12_DEPTH_STENCIL_DESC1 Graphics::DirectX12Utilities::CreateDepthStencilDesc1(bool enableZTest)
{
	D3D12_DEPTH_STENCIL_DESC1 desc{};
	desc.DepthEnable = enableZTest;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.StencilEnable = false;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	const D3D12_DEPTH_STENCILOP_DESC depthStencilOpDesc =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
	};

	desc.BackFace = depthStencilOpDesc;
	desc.FrontFace = depthStencilOpDesc;
	desc.DepthBoundsTestEnable = false;
	
	return desc;
}

D3D12_BLEND_DESC Graphics::DirectX12Utilities::CreateBlendDesc(DefaultBlendSetup setup)
{
	D3D12_BLEND_DESC desc{};

	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	if (setup == DefaultBlendSetup::BLEND_TRANSPARENT)
	{
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}
	else if (setup == DefaultBlendSetup::BLEND_ADDITIVE ||
		setup == DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE)
	{
		desc.RenderTarget[0].SrcBlend = setup == DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE ?
			D3D12_BLEND_SRC_ALPHA : D3D12_BLEND_ONE;

		desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}
	else
	{
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}

	return desc;
}

D3D12_SAMPLER_DESC Graphics::DirectX12Utilities::CreateSamplerDesc(DefaultFilterSetup setup)
{
	D3D12_SAMPLER_DESC desc{};

	if (setup == DefaultFilterSetup::FILTER_POINT_CLAMP || setup == DefaultFilterSetup::FILTER_POINT_WRAP)
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	else if (setup == DefaultFilterSetup::FILTER_BILINEAR_CLAMP || setup == DefaultFilterSetup::FILTER_BILINEAR_WRAP)
		desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	else if (setup == DefaultFilterSetup::FILTER_TRILINEAR_CLAMP || setup == DefaultFilterSetup::FILTER_TRILINEAR_WRAP)
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	else
		desc.Filter = D3D12_FILTER_ANISOTROPIC;

	if (setup == DefaultFilterSetup::FILTER_POINT_CLAMP || setup == DefaultFilterSetup::FILTER_BILINEAR_CLAMP ||
		setup == DefaultFilterSetup::FILTER_TRILINEAR_CLAMP || setup == DefaultFilterSetup::FILTER_ANISOTROPIC_CLAMP)
	{
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
	else
	{
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}

	if (setup == DefaultFilterSetup::FILTER_ANISOTROPIC_CLAMP || setup == DefaultFilterSetup::FILTER_ANISOTROPIC_WRAP)
		desc.MaxAnisotropy = 16u;
	else
		desc.MaxAnisotropy = 1u;

	desc.MaxLOD = 14.0f;

	return desc;
}
