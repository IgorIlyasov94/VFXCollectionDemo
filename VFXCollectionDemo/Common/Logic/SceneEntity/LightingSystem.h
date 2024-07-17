#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Renderer.h"
#include "LightDesc.h"

namespace Common::Logic::SceneEntity
{
	class LightingSystem final
	{
	public:
		LightingSystem(Graphics::DirectX12Renderer* renderer);
		~LightingSystem();

		LightID CreateLight(const LightDesc& desc);
		void SetSourceDesc(LightID id, const LightDesc& desc);
		const LightDesc& GetSourceDesc(LightID id) const;
		
		Graphics::Resources::ResourceID GetLightConstantBufferId() const;

		static constexpr uint32_t MAX_LIGHT_SOURCE_NUMBER = 32u;

	private:
		LightingSystem() = delete;

		void SetBufferElement(LightID id, const LightDesc& desc);

		struct LightElement
		{
			float3 position;
			uint32_t type;
			float3 color;
			float various0;
			float3 direction;
			float various1;
		};

		struct LightConstantBuffer
		{
			uint32_t lightSourcesNumber;
			float3 padding;
			LightElement lights[MAX_LIGHT_SOURCE_NUMBER];
		};

		Graphics::Resources::ResourceID lightConstantBufferId;
		
		std::vector<LightDesc> lights;
		uint32_t* lightSourcesNumberAddress;
		LightElement* lightBufferAddresses;
	};
}
