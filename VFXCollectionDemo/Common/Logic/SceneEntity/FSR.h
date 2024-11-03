#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"

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
		uint2 size;
		uint2 targetSize;
		float zNear;
		float zFar;
		float fovY;
		bool enableSharpening;
		float sharpnessFactor;
	};

	class FSR
	{
	public:
		FSR(ID3D12Device* device, const FSRDesc& desc);
		~FSR();

		void OnResize(ID3D12Device* device, const FSRDesc& desc);

		void UpdateJitter();
		float2 GetJitter() const;

		void ResetUpscaler();
		void Dispatch(ID3D12GraphicsCommandList* commandList, float deltaTime);

	private:
		FSR() = delete;

		void CreateContexts(ID3D12Device* device, const FSRDesc& desc);
		void DestroyContexts();

		uint32_t CalculateJitterPhaseCount(const uint2& size, const uint2& targetSize) const;
		void CacheJitterSequence(uint32_t jitterPhaseCount);
		float HaltonSequence(uint32_t index, uint32_t base);

#ifdef _DEBUG
		static void DebugMessage(uint32_t type, const wchar_t* message);
#endif

		static constexpr uint32_t AVERAGE_SEQUENCE_LENGTH = 32u;

		uint32_t jitterPhaseIndex;

		std::vector<float2> jitterSequence;

		ffx::DispatchDescUpscale upscaleDesc;
		ffx::Context upscaleContext;
	};
}
