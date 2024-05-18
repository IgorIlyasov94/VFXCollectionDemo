#include "ProcessHandler.h"

Common::ProcessHandler::ProcessHandler()
{

}

Common::ProcessHandler::~ProcessHandler()
{

}

int Common::ProcessHandler::Run()
{
	MSG message{};

	while (message.message != WM_QUIT)
	{
		PeekMessage(&message, nullptr, 0, 0, PM_REMOVE);

		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return static_cast<int>(message.wParam);
}
