#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"

namespace Common::Logic::SceneEntity
{
	using LightID = uint32_t;

	enum class LightType : uint32_t
	{
		DIRECTIONAL_LIGHT = 0u,
		POINT_LIGHT = 1u,
		AREA_LIGHT = 2u,
		SPOT_LIGHT = 3u,
		AMBIENT_LIGHT = 4u
	};

	struct LightDesc
	{
	public:
		float3 position;
		float radius;
		float3 color;
		float intensity;
		float3 direction;
		float cosPhi2;
		float cosTheta2;
		LightType type;
	};
}
