#include "Application.h"

Common::Application::Application(HINSTANCE instance, int cmdShow)
{
	RECT initialPlacement = { 0, 0, 1024, 768 };
	
	needBackgroundProcess = false;

	windowProcData = new WindowProcData();
	window = new Window(instance, cmdShow, initialPlacement, windowProcData->IsFullscreen(), WindowProc, windowProcData);

	int initialWidth = initialPlacement.right - initialPlacement.left;
	int initialHeight = initialPlacement.bottom - initialPlacement.top;
	renderer = new Graphics::DirectX12Renderer(window->GetHandler(), initialWidth, initialHeight, windowProcData->IsFullscreen());

	windowProcData->GetSceneManager().SetRenderer(renderer);
}

Common::Application::~Application()
{
	CleanUp();
}

int Common::Application::Run()
{
	try
	{
		MSG message{};

		for (;;)
		{
			for (;;)
			{
				if (needBackgroundProcess)
				{
					if (!PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
						break;
				}
				else
					GetMessage(&message, nullptr, 0, 0);

				if (message.message == WM_QUIT)
				{
					CleanUp();

					return static_cast<int>(message.wParam);
				}

				TranslateMessage(&message);
				DispatchMessage(&message);
			}

			BackgroundProcesses();
		}

		CleanUp();

		return static_cast<int>(message.wParam);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);

		CleanUp();

		return EXIT_FAILURE;
	}

	return 0;
}

void Common::Application::CleanUp()
{
	CommonUtility::FreeMemory(window);
	CommonUtility::FreeMemory(windowProcData);
}

void Common::Application::BackgroundProcesses()
{

}

LRESULT Common::Application::WindowProc(HWND windowHandler, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto data = reinterpret_cast<WindowProcData*>(GetWindowLongPtr(windowHandler, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(windowHandler, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));

		auto menuBar = CreateMenu();
		SetMenu(windowHandler, menuBar);

		return 0;
	}
	case WM_KEYDOWN:
	{
		auto keyData = static_cast<uint8_t>(wParam);

		return 0;
	}
	case WM_KEYUP:
	{
		auto keyData = static_cast<uint8_t>(wParam);

		return 0;
	}
	case WM_SYSKEYDOWN:
	{
		if ((wParam == VK_RETURN) && (lParam & (static_cast<int64_t>(1) << 29)))
		{
			if (data != nullptr)
			{
				data->ToggleFullscreen();
				data->GetSceneManager().SwitchFullscreenMode(data->IsFullscreen());

				return 0;
			}
		}

		break;
	}
	case WM_DISPLAYCHANGE:
	{
		return 0;
	}
	case WM_PAINT:
	{
		if (data != nullptr)
			data->GetSceneManager().OnFrameRender();

		return 0;
	}
	case WM_SIZE:
	{
		RECT windowSize;
		GetClientRect(windowHandler, &windowSize);

		if (data != nullptr)
			data->GetSceneManager().OnResize(windowSize.right - windowSize.left, windowSize.bottom - windowSize.top);
		
		return 0;
	}
	case WM_SETFOCUS:
	{
		if (data != nullptr)
			data->GetSceneManager().OnSetFocus();

		return 0;
	}
	case WM_KILLFOCUS:
	{
		if (data != nullptr)
			data->GetSceneManager().OnLostFocus();

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

		return 1;
	}
	default:
	{
		return DefWindowProc(windowHandler, message, wParam, lParam);
	}
	}

	return 0;
}
