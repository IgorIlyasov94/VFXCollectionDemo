#include "Window.h"

Common::Window::Window(HINSTANCE instance, int cmdShow, const RECT& initialPlacement, bool fullscreen, WNDPROC windowProc, void* procData)
{
	WNDCLASSEX windowClass{};

	auto windowClassName = L"VFX Collection Demo";
	auto windowTitleName = L"VFX Collection Demo";

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = windowProc;
	windowClass.hInstance = instance;
	windowClass.hIcon = LoadIcon(instance, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = LoadIcon(windowClass.hInstance, IDI_APPLICATION);

	RegisterClassExW(&windowClass);

	DWORD styles = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
	DWORD exStyles = WS_EX_APPWINDOW;

	RECT adjustedPlacement = initialPlacement;
	AdjustWindowRectEx(&adjustedPlacement, styles, true, exStyles);

	int initialX = adjustedPlacement.left;
	int initialY = adjustedPlacement.top;
	int initialWidth = adjustedPlacement.right - adjustedPlacement.left;
	int initialHeight = adjustedPlacement.bottom - adjustedPlacement.top;

	
	windowHandler = CreateWindowExW(WS_EX_APPWINDOW, windowClassName, windowTitleName, WS_OVERLAPPEDWINDOW,
		initialX, initialY, initialWidth, initialHeight, nullptr, nullptr, instance, procData);

	ShowWindow(windowHandler, SW_SHOWDEFAULT);
	UpdateWindow(windowHandler);

	SetForegroundWindow(windowHandler);
	SetFocus(windowHandler);

	ShowCursor(true);

	isFullscreen = fullscreen;
}

Common::Window::~Window()
{

}

HWND Common::Window::GetHandler()
{
	return windowHandler;
}
