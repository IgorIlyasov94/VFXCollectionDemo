#pragma once

#include "..\Includes.h"
#include "..\Graphics\DirectX12Renderer.h"
#include "Scene.h"

namespace Common
{
	class SceneManager final
	{
	public:
		SceneManager();
		~SceneManager();

		void SetRenderer(Graphics::DirectX12Renderer* renderer);

		void OnResize(uint32_t newWidth, uint32_t newHeight);
		void OnSetFocus();
		void OnLostFocus();
		void OnDeviceLost();

		void OnFrameRender();

		void SwitchFullscreenMode(bool enableFullscreen);

	private:
		Graphics::DirectX12Renderer* _renderer;
	};
}
