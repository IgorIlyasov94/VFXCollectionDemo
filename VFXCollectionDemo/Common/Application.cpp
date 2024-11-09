#include "Application.h"
#include "ProcessHandler.h"

Common::Application::Application(HINSTANCE instance, int cmdShow)
{
	RECT initialPlacement{ 20, 50, 1024, 768 };

	procedure = new WindowProcedure();
	window = new Window(instance, cmdShow, initialPlacement, false, procedure->Get());
	mainLogic = new Logic::MainLogic(initialPlacement, window->GetHandler(), false);

	SetWindowLongPtr(window->GetHandler(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(mainLogic));
}

Common::Application::~Application()
{
	delete mainLogic;
	delete procedure;
	delete window;
}

int Common::Application::Run()
{
	ProcessHandler processHandler{};
	return processHandler.Run(mainLogic);
}
