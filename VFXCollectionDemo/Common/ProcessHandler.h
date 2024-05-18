#pragma once

#include "../Includes.h"

namespace Common
{
	class ProcessHandler
	{
	public:
		ProcessHandler();
		~ProcessHandler();

		int Run();

	private:
		ProcessHandler(const ProcessHandler&) = delete;
		ProcessHandler(ProcessHandler&&) = delete;
		ProcessHandler& operator=(const ProcessHandler&) = delete;
		ProcessHandler& operator=(ProcessHandler&&) = delete;
	};
}
