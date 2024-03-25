#pragma once

#include "..\Includes.h"
#include "CommonUtilities.h"
#include "Window.h"
#include "SceneManager.h"

namespace Common
{
	class Application final
	{
	public:
		Application(HINSTANCE instance, int cmdShow);
		~Application();

		int Run();

	private:
		Application() = delete;

		void CleanUp();
		void BackgroundProcesses();

		static LRESULT CALLBACK WindowProc(HWND windowHandler, UINT message, WPARAM wParam, LPARAM lParam);

		struct WindowProcData
		{
		public:
			WindowProcData()
				: isFullscreen(false)
			{
				sceneManager = new SceneManager();
			}

			~WindowProcData()
			{
				CommonUtility::FreeMemory(sceneManager);
			}

			bool IsFullscreen()
			{
				return isFullscreen;
			}

			void SetFullscreen(bool enable)
			{
				isFullscreen = enable;
			}

			void ToggleFullscreen()
			{
				isFullscreen = !isFullscreen;
			}

			SceneManager& GetSceneManager()
			{
				return *sceneManager;
			}

		private:
			bool isFullscreen;
			SceneManager* sceneManager;
		};

		bool needBackgroundProcess;

		Window* window;
		Graphics::DirectX12Renderer* renderer;
		WindowProcData* windowProcData;
	};
}
