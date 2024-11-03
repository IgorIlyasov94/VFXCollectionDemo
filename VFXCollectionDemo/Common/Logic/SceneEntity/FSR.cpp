#include "FSR.h"
#include <ffx_api/dx12/ffx_api_dx12.hpp>

Common::Logic::SceneEntity::FSR::FSR(ID3D12Device* device, const FSRDesc& desc)
	: upscaleDesc{}, upscaleContext{}
{
	jitterSequence.reserve(AVERAGE_SEQUENCE_LENGTH);

	CreateContexts(device, desc);
}

Common::Logic::SceneEntity::FSR::~FSR()
{
	DestroyContexts();
}

void Common::Logic::SceneEntity::FSR::OnResize(ID3D12Device* device, const FSRDesc& desc)
{
	DestroyContexts();
	CreateContexts(device, desc);
}

void Common::Logic::SceneEntity::FSR::UpdateJitter()
{
	upscaleDesc.jitterOffset.x = jitterSequence[jitterPhaseIndex].x;
	upscaleDesc.jitterOffset.y = jitterSequence[jitterPhaseIndex].y;

	jitterPhaseIndex = (++jitterPhaseIndex) % jitterSequence.size();
}

float2 Common::Logic::SceneEntity::FSR::GetJitter() const
{
	return float2(upscaleDesc.jitterOffset.x, upscaleDesc.jitterOffset.y);
}

void Common::Logic::SceneEntity::FSR::ResetUpscaler()
{
	jitterPhaseIndex = 0u;

	upscaleDesc.reset = true;
}

void Common::Logic::SceneEntity::FSR::Dispatch(ID3D12GraphicsCommandList* commandList, float deltaTime)
{
	upscaleDesc.commandList = commandList;
	upscaleDesc.frameTimeDelta = deltaTime * 1000.0f;
	
	ffx::Dispatch(upscaleContext, upscaleDesc);

	upscaleDesc.reset = false;
}

void Common::Logic::SceneEntity::FSR::CreateContexts(ID3D12Device* device, const FSRDesc& desc)
{
	upscaleDesc.color = ffxApiGetResourceDX12(desc.colorBuffer, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.depth = ffxApiGetResourceDX12(desc.depthTarget, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.motionVectors = ffxApiGetResourceDX12(desc.motionTarget, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.exposure = ffxApiGetResourceDX12(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.reactive = ffxApiGetResourceDX12(desc.reactiveMask, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.transparencyAndComposition = ffxApiGetResourceDX12(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	upscaleDesc.output = ffxApiGetResourceDX12(desc.outputBuffer, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

	upscaleDesc.renderSize.width = desc.size.x;
	upscaleDesc.renderSize.height = desc.size.y;
	upscaleDesc.upscaleSize.width = desc.targetSize.x;
	upscaleDesc.upscaleSize.height = desc.targetSize.y;
	upscaleDesc.cameraNear = desc.zNear;
	upscaleDesc.cameraFar = desc.zFar;
	upscaleDesc.cameraFovAngleVertical = desc.fovY;
	upscaleDesc.motionVectorScale = { -static_cast<float>(desc.size.x), static_cast<float>(desc.size.y) };

	upscaleDesc.enableSharpening = desc.enableSharpening;
	upscaleDesc.sharpness = desc.sharpnessFactor;
	upscaleDesc.preExposure = 1.0f;
	upscaleDesc.viewSpaceToMetersFactor = 1.0f;
	upscaleDesc.reset = true;
	upscaleDesc.flags = FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;

	auto jitterPhaseCount = CalculateJitterPhaseCount(desc.size, desc.targetSize);
	CacheJitterSequence(jitterPhaseCount);

	jitterPhaseIndex = 0u;

	ffx::CreateBackendDX12Desc createBackendDesc{};
	createBackendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
	createBackendDesc.device = device;

	ffx::CreateContextDescUpscale createContextDescUpscale{};
	createContextDescUpscale.flags = FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
	createContextDescUpscale.maxRenderSize.width = desc.maxSize.x;
	createContextDescUpscale.maxRenderSize.height = desc.maxSize.y;
	createContextDescUpscale.maxUpscaleSize.width = desc.maxSize.x;
	createContextDescUpscale.maxUpscaleSize.height = desc.maxSize.y;

#ifdef _DEBUG
	createContextDescUpscale.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
	createContextDescUpscale.fpMessage = &Common::Logic::SceneEntity::FSR::DebugMessage;
#endif

	ffx::CreateContext(upscaleContext, nullptr, createContextDescUpscale, createBackendDesc);
}

void Common::Logic::SceneEntity::FSR::DestroyContexts()
{
	if (upscaleContext != nullptr)
	{
		ffx::DestroyContext(upscaleContext);
		upscaleContext = nullptr;
	}
}

uint32_t Common::Logic::SceneEntity::FSR::CalculateJitterPhaseCount(const uint2& size, const uint2& targetSize) const
{
	auto diagonal = static_cast<float>(std::sqrt(size.x * size.x + size.y * size.y));
	auto targetDiagonal = static_cast<float>(std::sqrt(targetSize.x * targetSize.x + targetSize.y * targetSize.y));

	auto sizeFactorSqr = targetDiagonal / diagonal;
	sizeFactorSqr *= sizeFactorSqr;

	return static_cast<uint32_t>(std::ceil(8.0f * sizeFactorSqr));
}

void Common::Logic::SceneEntity::FSR::CacheJitterSequence(uint32_t jitterPhaseCount)
{
	jitterSequence.clear();

	for (uint32_t index = 1u; index <= jitterPhaseCount; index++)
	{
		float2 jitter{};
		jitter.x = HaltonSequence(index, 2u) - 0.5f;
		jitter.y = HaltonSequence(index, 3u) - 0.5f;

		jitterSequence.push_back(std::move(jitter));
	}
}

float Common::Logic::SceneEntity::FSR::HaltonSequence(uint32_t index, uint32_t base)
{
	auto currentIndex = index;
	auto sumElement = 1.0f;
	auto result = 0.0f;

	while (currentIndex > 0u)
	{
		currentIndex = static_cast<uint32_t>(std::floor(currentIndex / static_cast<float>(base)));
		sumElement /= static_cast<float>(base);

		result += sumElement * static_cast<float>(currentIndex % base);
	}

	return result;
}

#ifdef _DEBUG
void Common::Logic::SceneEntity::FSR::DebugMessage(uint32_t type, const wchar_t* message)
{
	if (type == FFX_API_MESSAGE_TYPE_ERROR)
	{
		std::wstring messageString = L"FSR_API_DEBUG_ERROR: ";
		messageString += message;
		messageString += L'\n';

		OutputDebugString(messageString.c_str());
	}
	else if (type == FFX_API_MESSAGE_TYPE_WARNING)
	{
		std::wstring messageString = L"FSR_API_DEBUG_WARNING: ";
		messageString += message;
		messageString += L'\n';

		OutputDebugString(messageString.c_str());
	}
}
#endif
