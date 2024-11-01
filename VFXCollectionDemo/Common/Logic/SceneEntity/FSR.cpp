#include "FSR.h"
#include <ffx_api/dx12/ffx_api_dx12.hpp>

Common::Logic::SceneEntity::FSR::FSR(ID3D12Device* device, const FSRDesc& desc)
	: upscaleDesc{}, upscaleContext{}
{
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

void Common::Logic::SceneEntity::FSR::ResetUpscaler()
{
	upscaleDesc.reset = true;
}

void Common::Logic::SceneEntity::FSR::Dispatch(ID3D12GraphicsCommandList* commandList, const uint2& size, const uint2& targetSize,
	const Camera& camera, float2 jitterOffset, float deltaTime)
{
	upscaleDesc.commandList = commandList;
	upscaleDesc.jitterOffset.x = jitterOffset.x;
	upscaleDesc.jitterOffset.y = jitterOffset.y;
	upscaleDesc.frameTimeDelta = deltaTime * 1000.0f;
	upscaleDesc.renderSize.width = size.x;
	upscaleDesc.renderSize.height = size.y;
	upscaleDesc.upscaleSize.width = targetSize.x;
	upscaleDesc.upscaleSize.height = targetSize.y;
	upscaleDesc.cameraNear = camera.GetZNear();
	upscaleDesc.cameraFar = camera.GetZFar();
	upscaleDesc.cameraFovAngleVertical = camera.GetFovY();

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

	upscaleDesc.motionVectorScale = { 1.0f, 1.0f };
	upscaleDesc.enableSharpening = desc.enableSharpening;
	upscaleDesc.sharpness = desc.sharpnessFactor;
	upscaleDesc.preExposure = 1.0f;
	upscaleDesc.viewSpaceToMetersFactor = 1.0f;
	upscaleDesc.reset = true;
	upscaleDesc.flags = FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;

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
