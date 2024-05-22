#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"
#include "../../../Graphics/DirectX12Renderer.h"

namespace Common::Logic::Scene
{
	using SceneID = uint32_t;

	class IScene
	{
	public:
		virtual ~IScene() = 0 {};

		virtual void Load(Graphics::DirectX12Renderer* renderer) = 0;
		virtual void Unload(Graphics::DirectX12Renderer* renderer) = 0;

		virtual void OnResize(uint32_t newWidth, uint32_t newHeight) = 0;

		virtual void Update() = 0;
		virtual void Render(ID3D12GraphicsCommandList* commandList) = 0;
		virtual void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) = 0;

		virtual bool IsLoaded() = 0;
	};
}
