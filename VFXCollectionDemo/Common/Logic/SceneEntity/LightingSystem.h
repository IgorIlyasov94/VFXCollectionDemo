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
		Graphics::Resources::ResourceID CreateLightParticleBuffer(ID3D12GraphicsCommandList* commandList,
			uint32_t particleNumber);
		Graphics::Resources::ResourceID GetLightParticleBuffer();

		void UpdateSourceDesc(LightID id);
		LightDesc& GetSourceDesc(LightID id);
		
		uint32_t GetLightMatricesNumber() const;

		Graphics::Resources::ResourceID GetLightConstantBufferId();
		Graphics::Resources::ResourceID GetLightMatricesConstantBufferId();

		void BeforeStartRenderShadowMaps(ID3D12GraphicsCommandList* commandList);
		void StartRenderShadowMap(LightID id, ID3D12GraphicsCommandList* commandList);

		void EndRenderShadowMaps(ID3D12GraphicsCommandList* commandList);
		void EndUsingShadowMaps(ID3D12GraphicsCommandList* commandList);

		void Clear();

		static constexpr uint32_t SHADOW_MAP_SIZE = 2048u;
		static constexpr uint32_t CUBE_SHADOW_MAP_SIZE = 1024u;

		static constexpr float SHADOW_MAP_Z_NEAR = 0.01f;
		static constexpr float SHADOW_MAP_Z_FAR = 1000.0f;

	private:
		LightingSystem() = delete;

		uint32_t GetSizeOfType(LightType type);
		uint32_t CalculateLightBufferSize();

		void SetLightBufferElement(LightDesc& desc);

		Graphics::Resources::ResourceID CreateShadowMap(bool isCube);
		void CreateConstantBuffers();

		void SetupViewProjectMatrices(LightDesc& desc);
		float4x4 BuildViewProjectMatrix(const float3& position, const float3& direction,
			const float3& up, uint32_t size, bool isDirectional);

		D3D12_VIEWPORT viewport;
		D3D12_VIEWPORT cubeViewport;
		D3D12_RECT scissorRectangle;
		D3D12_RECT cubeScissorRectangle;

		bool isLightConstantBufferBuilded;
		bool isLightParticleBufferBuilded;
		uint32_t lightParticleNumber;
		uint32_t lightMatricesNumber;

		std::vector<LightDesc> lights;
		std::vector<D3D12_RESOURCE_BARRIER> barriers;

		Graphics::Resources::ResourceID lightConstantBufferId;
		Graphics::Resources::ResourceID lightParticleBufferId;
		Graphics::Resources::ResourceID lightMatricesConstantBufferId;

		Graphics::DirectX12Renderer* _renderer;
	};
}
