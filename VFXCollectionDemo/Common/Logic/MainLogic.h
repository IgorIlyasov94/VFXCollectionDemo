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

		void ToggleFullscreen(HWND windowHandler);
		void SetFullscreen(bool isFullscreen, HWND windowHandler);

		void OnResize(uint32_t newWidth, uint32_t newHeight, HWND windowHandler);
		void OnSetFocus(HWND windowHandler);
		void OnLostFocus(HWND windowHandler);

		void Update();
		void Render();

		bool NeedBackgroundUpdate() const;

	private:
		MainLogic() = delete;
		MainLogic(const MainLogic&) = delete;
		MainLogic(MainLogic&&) = delete;
		MainLogic& operator=(const MainLogic&) = delete;
		MainLogic& operator=(MainLogic&&) = delete;

		Scene::SceneID luxSceneId;
		Scene::SceneID whiteroomSceneId;

		SceneManager* sceneManager;
		Graphics::DirectX12Renderer* renderer;
		bool needBackgroundUpdate;
	};
}
