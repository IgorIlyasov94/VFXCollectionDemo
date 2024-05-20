#pragma once

#include "../Includes.h"

namespace Common
{
	class Window
	{
	public:
		Window(HINSTANCE instance, int cmdShow, const RECT& initialPlacement, bool fullscreen, WNDPROC windowProc);
		~Window();

		HWND GetHandler();

	private:
		Window() = delete;
		Window(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(const Window&) = delete;
		Window& operator=(Window&&) = delete;

		HWND windowHandler;
		bool isFullscreen;
	};
}
