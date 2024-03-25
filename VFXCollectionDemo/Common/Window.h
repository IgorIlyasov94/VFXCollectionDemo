#pragma once

#include "..\Includes.h"
#include "..\Graphics\DirectX12Renderer.h"

namespace Common
{
	class Window final
	{
	public:
		Window(HINSTANCE instance, int cmdShow, const RECT& initialPlacement, bool fullscreen, WNDPROC windowProc, void* procData);
		~Window();

		HWND GetHandler();

	private:
		Window() = delete;

		HWND windowHandler;

		bool isFullscreen;
	};
}
