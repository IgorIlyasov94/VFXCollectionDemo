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
		//case WM_EXITSIZEMOVE:
		//{
		//	//if (mainLogic != nullptr)
		//	//{
		//	//	DWORD styles = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
		//	//	DWORD exStyles = WS_EX_APPWINDOW;
		//	//
		//	//	RECT adjustedPlacement{};
		//	//	GetClientRect(windowHandler, &adjustedPlacement);
		//	//	AdjustWindowRectEx(&adjustedPlacement, styles, true, exStyles);
		//	//
		//	//	uint32_t newWidth = adjustedPlacement.right - adjustedPlacement.left;
		//	//	uint32_t newHeight = adjustedPlacement.bottom - adjustedPlacement.top;
		//	//
		//	//	mainLogic->OnResize(newWidth, newHeight, windowHandler);
		//	//}
		//
		//	DWORD styles = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
		//	DWORD exStyles = WS_EX_APPWINDOW;
		//
		//	RECT adjustedPlacement{};
		//	GetClientRect(windowHandler, &adjustedPlacement);
		//	AdjustWindowRectEx(&adjustedPlacement, styles, true, exStyles);
		//
		//	//InvalidateRect(windowHandler, &adjustedPlacement, true);
		//	//UpdateWindow(windowHandler);
		//
		//	RedrawWindow(windowHandler, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE);
		//
		//	return 0;
		//}
		case WM_SIZE:
		{
			if (mainLogic != nullptr)
			{
				uint32_t newWidth = static_cast<uint32_t>(lParam) & 0xffffu;
				uint32_t newHeight = static_cast<uint32_t>(lParam >> 16u) & 0xffffu;

				mainLogic->OnResize(newWidth, newHeight, windowHandler);
			}

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
		/*case WM_SETFOCUS:
		{
			if (data != nullptr)
			{
				auto renderer = data->renderer;

				renderer->OnSetFocus(windowHandler);
			}

			return 0;
		}
		case WM_KILLFOCUS:
		{
			if (data != nullptr)
			{
				auto renderer = data->renderer;

				renderer->OnLostFocus(windowHandler);
			}

			return 0;
		}*/
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
}
