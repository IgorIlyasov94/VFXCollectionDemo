#pragma once

#include "DirectX12Includes.h"
#include "Resources/IResourceDesc.h"

namespace Graphics
{
	enum class DefaultBlendSetup
	{
		BLEND_OPAQUE = 0u,
		BLEND_TRANSPARENT = 1u,
		BLEND_ADDITIVE = 2u,
		BLEND_PREMULT_ALPHA_ADDITIVE = 3u,
	};

	enum class DefaultFilterSetup
	{
		FILTER_POINT_CLAMP = 0u,
		FILTER_BILINEAR_CLAMP = 1u,
		FILTER_TRILINEAR_CLAMP = 2u,
		FILTER_ANISOTROPIC_CLAMP = 3u,
		FILTER_POINT_WRAP = 4u,
		FILTER_BILINEAR_WRAP = 5u,
		FILTER_TRILINEAR_WRAP = 6u,
		FILTER_ANISOTROPIC_WRAP = 7u,
		FILTER_COMPARISON_POINT_CLAMP = 8u,
		FILTER_COMPARISON_BILINEAR_CLAMP = 9u,
		FILTER_COMPARISON_TRILINEAR_CLAMP = 10u,
		FILTER_COMPARISON_ANISOTROPIC_CLAMP = 11u,
		FILTER_COMPARISON_POINT_WRAP = 12u,
		FILTER_COMPARISON_BILINEAR_WRAP = 13u,
		FILTER_COMPARISON_TRILINEAR_WRAP = 14u,
		FILTER_COMPARISON_ANISOTROPIC_WRAP = 15u
	};

	enum class DefaultFilterComparisonFunc
	{
		COMPARISON_NEVER = 0u,
		COMPARISON_LESS = 1u,
		COMPARISON_EQUAL = 2u,
		COMPARISON_LESS_EQUAL = 3u,
		COMPARISON_GREATER = 4u,
		COMPARISON_NOT_EQUAL = 5u,
		COMPARISON_GREATER_EQUAL = 6u,
		COMPARISON_ALWAYS = 7u
	};

	class DirectX12Utilities final
	{
	public:
		static void WaitForGPU(ID3D12CommandQueue* queue, ID3D12Fence* fence, HANDLE& fenceEvent, uint64_t& fenceValue);

		static D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDesc(const Resources::TextureDesc& desc);
		static D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDesc(const Resources::BufferDesc& desc);
		static D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUAVDesc(const Resources::TextureDesc& desc);
		static D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUAVDesc(const Resources::BufferDesc& desc);
		static D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDesc(const Resources::TextureDesc& desc);
		static D3D12_DEPTH_STENCIL_VIEW_DESC CreateDSVDesc(const Resources::TextureDesc& desc);

		static D3D12_RASTERIZER_DESC CreateRasterizeDesc(D3D12_CULL_MODE mode, float depthBias = 0.0f);
		static D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc(bool enableZTest, bool readOnly = false);
		static D3D12_DEPTH_STENCIL_DESC1 CreateDepthStencilDesc1(bool enableZTest, bool readOnly = false);

		static D3D12_BLEND_DESC CreateBlendDesc(DefaultBlendSetup setup);
		static D3D12_SAMPLER_DESC CreateSamplerDesc(DefaultFilterSetup setup,
			DefaultFilterComparisonFunc comparisonFunc = DefaultFilterComparisonFunc::COMPARISON_NEVER);

	private:
		DirectX12Utilities() = delete;
		~DirectX12Utilities() = delete;
		DirectX12Utilities(const DirectX12Utilities&) = delete;
		DirectX12Utilities(DirectX12Utilities&&) = delete;
		DirectX12Utilities& operator=(const DirectX12Utilities&) = delete;
		DirectX12Utilities& operator=(DirectX12Utilities&&) = delete;
	};
}
