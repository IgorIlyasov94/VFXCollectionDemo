#pragma once

#include "../Includes.h"
#include "../Graphics/DirectX12Renderer.h"

namespace Common
{
	struct WindowProcedureData
	{
		Graphics::DirectX12Renderer* renderer;
	};

	class WindowProcedure
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
