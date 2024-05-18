#include "Application.h"
#include "ProcessHandler.h"

Common::Application::Application(HINSTANCE instance, int cmdShow)
{
	RECT initialPlacement{ 50, 70, 1024, 768 };

	procedure = new WindowProcedure();
	window = new Window(instance, cmdShow, initialPlacement, false, procedure->Get());
	procedureData.renderer = new Graphics::DirectX12Renderer(initialPlacement, window->GetHandler(), false);

	SetWindowLongPtr(window->GetHandler(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&procedureData));
}

Common::Application::~Application()
{
	delete procedureData.renderer;
	delete procedure;
	delete window;
}

int Common::Application::Run()
{
	ProcessHandler processHandler{};
	return processHandler.Run();
}
