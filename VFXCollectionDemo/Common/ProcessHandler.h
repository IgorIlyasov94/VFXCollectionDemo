#pragma once

#include "../Includes.h"
#include "Logic/MainLogic.h"

namespace Common
{
	class ProcessHandler
	{
	public:
		ProcessHandler();
		~ProcessHandler();

		int Run(Logic::MainLogic* mainLogic);

	private:
		ProcessHandler(const ProcessHandler&) = delete;
		ProcessHandler(ProcessHandler&&) = delete;
		ProcessHandler& operator=(const ProcessHandler&) = delete;
		ProcessHandler& operator=(ProcessHandler&&) = delete;
	};
}
