#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/Resources/ResourceManager.h"

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

	struct DirectionalLight
	{
	public:
		float3 direction;
		float padding0;
		float3 color;
		float padding1;
	};

	struct PointLight
	{
	public:
		float3 position;
		float range;
		float3 color;
		float padding1;
	};

	struct AreaLight
	{
	public:
		float3 position;
		float radius;
		float3 color;
		float range;
	};

	struct SpotLight
	{
	public:
		float3 position;
		float range;
		float3 color;
		float cosPhi2;
		float3 direction;
		float cosTheta2;
	};

	struct AmbientLight
	{
	public:
		float3 color;
		float padding;
	};

	struct LightDesc
	{
	public:
		uint32_t GetLightMatrixStartIndex() const
		{
			return lightMatrixStartIndex;
		}

		Graphics::Resources::ResourceID GetShadowMapId() const
		{
			return shadowMapId;
		}

		float3 position;
		float radius;
		float3 color;
		float intensity;
		float3 direction;
		float cosPhi2;
		float cosTheta2;
		float range;
		bool castShadows;
		LightType type;

	private:
		Graphics::Resources::ResourceID shadowMapId;
		D3D12_CPU_DESCRIPTOR_HANDLE shadowMapCPUDescriptor;
		Graphics::Resources::GPUResource* shadowMapResource;
		uint32_t lightMatrixStartIndex;
		void* lightBufferStartAddress;
		float4x4* viewProjections;

		friend class LightingSystem;
	};
}
