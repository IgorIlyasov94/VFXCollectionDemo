#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"
#include "Camera.h"

#include <ffx_api/ffx_upscale.hpp>

#pragma comment(lib, "amd_fidelityfx_dx12.lib")

namespace Common::Logic::SceneEntity
{
	struct FSRDesc
	{
		ID3D12Resource* colorBuffer;
		ID3D12Resource* depthTarget;
		ID3D12Resource* motionTarget;
		ID3D12Resource* reactiveMask;
		ID3D12Resource* outputBuffer;
		uint2 maxSize;
		bool enableSharpening;
		float sharpnessFactor;
	};

	class FSR
	{
	public:
		FSR(ID3D12Device* device, const FSRDesc& desc);
		~FSR();

		void OnResize(ID3D12Device* device, const FSRDesc& desc);

		void ResetUpscaler();
		void Dispatch(ID3D12GraphicsCommandList* commandList, const uint2& size, const uint2& targetSize,
			const Camera& camera, float2 jitterOffset, float deltaTime);

	private:
		FSR() = delete;

		void CreateContexts(ID3D12Device* device, const FSRDesc& desc);
		void DestroyContexts();

		static void DebugMessage(uint32_t type, const wchar_t* message);

		ffx::DispatchDescUpscale upscaleDesc;
		ffx::Context upscaleContext;
	};
}
