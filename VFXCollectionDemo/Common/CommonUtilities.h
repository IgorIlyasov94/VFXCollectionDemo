#pragma once

#include "..\Includes.h"

namespace Common
{
	class CommonUtility
	{
	public:
		template<typename T>
		static constexpr const T AlignSize(const T size, const T alignment) noexcept
		{
			return (size + alignment - 1) & ~(alignment - 1);
		}

		template<typename T>
		static bool CheckEnum(const T enumObject, const T enumValue) noexcept
		{
			return (enumObject & enumValue) == enumValue;
		}

		template<typename T>
		static void FreeMemory(T*& pointer)
		{
			if (pointer != nullptr)
			{
				delete pointer;
				pointer = nullptr;
			}
		}

	private:
	};
}
