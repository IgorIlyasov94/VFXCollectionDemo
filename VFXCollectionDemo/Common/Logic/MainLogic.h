#pragma once

#include "../../Includes.h"
#include "SceneManager.h"
#include "../../Graphics/DirectX12Renderer.h"

namespace Common::Logic
{
	class MainLogic final
	{
	public:
		MainLogic(const RECT& windowPlacement, HWND windowHandler, bool isFullscreen);
		~MainLogic();

		void OnResize(uint32_t newWidth, uint32_t newHeight, HWND windowHandler);
		void OnSetFocus(HWND windowHandler);
		void OnLostFocus(HWND windowHandler);

		void Update();
		void Render();

		bool NeedBackgroundUpdate();

	private:
		MainLogic() = delete;
		MainLogic(const MainLogic&) = delete;
		MainLogic(MainLogic&&) = delete;
		MainLogic& operator=(const MainLogic&) = delete;
		MainLogic& operator=(MainLogic&&) = delete;

		SceneManager* sceneManager;
		Graphics::DirectX12Renderer* renderer;
		bool _isFullscreen;
		bool needBackgroundUpdate;
	};
}
