#pragma once

#include "DirectX12Includes.h"
#include "Resources/IResourceDesc.h"

namespace Graphics
{
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

		static D3D12_RASTERIZER_DESC CreateRasterizeDesc(D3D12_CULL_MODE mode);
		static D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc(bool enableZTest);
		static D3D12_DEPTH_STENCIL_DESC1 CreateDepthStencilDesc1(bool enableZTest);

	private:
		DirectX12Utilities() = delete;
		~DirectX12Utilities() = delete;
		DirectX12Utilities(const DirectX12Utilities&) = delete;
		DirectX12Utilities(DirectX12Utilities&&) = delete;
		DirectX12Utilities& operator=(const DirectX12Utilities&) = delete;
		DirectX12Utilities& operator=(DirectX12Utilities&&) = delete;
	};
}
