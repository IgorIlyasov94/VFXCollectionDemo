#pragma once

#include "../Includes.h"
#include "Logic/MainLogic.h"

namespace Common
{
	class WindowProcedure final
	{
	public:
		WindowProcedure();
		~WindowProcedure();

		WNDPROC Get();

	private:
		WindowProcedure(const WindowProcedure&) = delete;
		WindowProcedure(WindowProcedure&&) = delete;
		WindowProcedure& operator=(const WindowProcedure&) = delete;
		WindowProcedure& operator=(WindowProcedure&&) = delete;

		static void CreateMenuBar(HWND windowHandler);

		static LRESULT Procedure(HWND windowHandler, UINT message, WPARAM wParam, LPARAM lParam);
	};
}
