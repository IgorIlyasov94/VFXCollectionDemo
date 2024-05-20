#include "ProcessHandler.h"

Common::ProcessHandler::ProcessHandler()
{

}

Common::ProcessHandler::~ProcessHandler()
{

}

int Common::ProcessHandler::Run(Logic::MainLogic* mainLogic)
{
	MSG message{};

	for (;;)
	{
		for (;;)
		{
			if (mainLogic != nullptr && mainLogic->NeedBackgroundUpdate())
			{
				if (!PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
					break;
			}
			else
				GetMessage(&message, nullptr, 0, 0);

			if (message.message == WM_QUIT)
				return static_cast<int>(message.wParam);

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		mainLogic->Update();
	}

	return static_cast<int>(message.wParam);
}
