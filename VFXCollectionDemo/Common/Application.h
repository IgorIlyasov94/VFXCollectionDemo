#pragma once

#include "../Includes.h"
#include "WindowProcedure.h"
#include "Window.h"
#include "../Graphics/DirectX12Renderer.h"

namespace Common
{
	class Application
	{
	public:
		Application(HINSTANCE instance, int cmdShow);
		~Application();

		int Run();

	private:
		Application() = delete;
		Application(const Application&) = delete;
		Application(Application&&) = delete;
		Application& operator=(const Application&) = delete;
		Application& operator=(Application&&) = delete;

		WindowProcedure* procedure;
		Window* window;
		
		WindowProcedureData procedureData;
	};
}
