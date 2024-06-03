#pragma once

#include "../Includes.h"
#include "../Graphics/DirectX12Includes.h"

namespace Common
{
	class Utilities
	{
	public:
		template<typename T>
		static float Random(T& generator)
		{
			return std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);
		}

		template<typename T>
		static float2 Random2(T& generator)
		{
			float2 random
			{
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator)
			};

			return random;
		}

		template<typename T>
		static float3 Random3(T& generator)
		{
			float3 random
			{
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator)
			};

			return random;
		}

		template<typename T>
		static float4 Random4(T& generator)
		{
			float4 random
			{
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator),
				std::generate_canonical<float, std::numeric_limits<float>::digits>(generator)
			};

			return random;
		}

	private:
		Utilities() = delete;
		~Utilities() = delete;
		Utilities(const Utilities&) = delete;
		Utilities(Utilities&&) = delete;
		Utilities& operator=(const Utilities&) = delete;
		Utilities& operator=(Utilities&&) = delete;
	};
}
