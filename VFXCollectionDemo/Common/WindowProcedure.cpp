#include "WindowProcedure.h"

Common::WindowProcedure::WindowProcedure()
{

}

Common::WindowProcedure::~WindowProcedure()
{

}

WNDPROC Common::WindowProcedure::Get()
{
	return &Procedure;
}

void Common::WindowProcedure::CreateMenuBar(HWND windowHandler)
{
	HMENU menuBar = CreateMenu();
	AppendMenuW(menuBar, MF_SEPARATOR, 0, nullptr);
	SetMenu(windowHandler, menuBar);
}

LRESULT Common::WindowProcedure::Procedure(HWND windowHandler, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto mainLogic = reinterpret_cast<Logic::MainLogic*>(GetWindowLongPtr(windowHandler, GWLP_USERDATA));

	static uint32_t oldWidth = 1u;
	static uint32_t oldHeight = 1u;
	static uint32_t newWidth = 1u;
	static uint32_t newHeight = 1u;

	switch (message)
	{
		//case WM_CREATE:
		//{
		//	LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		//	SetWindowLongPtr(windowHandler, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
		//
		//	CreateMenuBar(windowHandler);
		//
		//	return 0;
		//}
		case WM_SYSKEYDOWN:
		{
			if ((wParam == VK_RETURN) && (lParam & (1ull << 29ull)))
			{
				if (mainLogic != nullptr)
					mainLogic->ToggleFullscreen(windowHandler);
			}

			break;
		}
		case WM_KEYDOWN:
		{
			auto keyData = static_cast<uint8_t>(wParam);

			if (keyData == VK_ESCAPE)
				PostQuitMessage(0);

			return 0;
		}
		case WM_KEYUP:
		{
			auto keyData = static_cast<uint8_t>(wParam);

			return 0;
		}
		case WM_EXITSIZEMOVE:
		{
			if (mainLogic != nullptr && (oldWidth != newWidth || oldHeight != newHeight))
				mainLogic->OnResize(newWidth, newHeight, windowHandler);

			return 0;
		}
		case WM_SIZE:
		{
			newWidth = static_cast<uint32_t>(lParam) & 0xffffu;
			newHeight = static_cast<uint32_t>(lParam >> 16u) & 0xffffu;

			return 0;
		}
		case WM_ERASEBKGND:
		{
			if (mainLogic != nullptr)
			{
				mainLogic->Update();
				mainLogic->Render();
			}

			return 1;
		}
		case WM_PAINT:
		{
			if (mainLogic != nullptr)
			{
				mainLogic->Update();
				mainLogic->Render();
			}

			return 0;
		}
		case WM_SETFOCUS:
		{
			if (mainLogic != nullptr)
				mainLogic->OnSetFocus(windowHandler);

			return 0;
		}
		case WM_KILLFOCUS:
		{
			if (mainLogic != nullptr)
				mainLogic->OnLostFocus(windowHandler);

			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);

			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);

			return 0;
		}
		default:
		{
			return DefWindowProc(windowHandler, message, wParam, lParam);
		}
	}

	return 0;
}
