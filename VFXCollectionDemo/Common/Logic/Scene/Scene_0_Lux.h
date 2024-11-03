#pragma once

#include "../../../Includes.h"

#include "../SceneEntity/IDrawable.h"

#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/Assets/Material.h"

#include "../../../Common/Logic/SceneEntity/PostProcessManager.h"

#include "../SceneEntity/Camera.h"
#include "../SceneEntity/Terrain.h"
#include "../SceneEntity/VegetationSystem.h"
#include "../SceneEntity/LightingSystem.h"

#include "IScene.h"

namespace Common::Logic::Scene
{
	class Scene_0_Lux : public IScene
	{
	public:
		Scene_0_Lux();
		~Scene_0_Lux() override;

		void Load(Graphics::DirectX12Renderer* renderer) override;
		void Unload(Graphics::DirectX12Renderer* renderer) override;

		void OnResize(Graphics::DirectX12Renderer* renderer) override;

		void Update() override;
		void RenderShadows(ID3D12GraphicsCommandList* commandList) override;
		void Render(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer) override;
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) override;

		bool IsLoaded() override;

	private:
		void CreateConstantBuffers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			uint32_t width, uint32_t height);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateLights(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::DirectX12Renderer* renderer);
		void CreateObjects(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer);

		bool isLoaded;
		
		static constexpr float FOV_Y = DirectX::XM_PI / 4.0f;
		static constexpr float Z_NEAR = 0.1f;
		static constexpr float Z_FAR = 1000.0f;

		static constexpr float AREA_LIGHT_INTENSITY = 15.0f;
		static constexpr float AREA_LIGHT_RANGE = 8.0f;

		static constexpr float AMBIENT_LIGHT_INTENSITY = 0.15f;

		static constexpr bool DEPTH_PREPASS_ENABLED = true;
		static constexpr bool FSR_ENABLED = true;
		static constexpr bool MOTION_BLUR_ENABLED = true;
		static constexpr bool VOLUMETRIC_FOG_ENABLED = true;
		static constexpr bool USING_PARTICLE_LIGHT = true;

		static constexpr float WHITE_CUTOFF = 1.7f;
		static constexpr float BRIGHT_THRESHOLD = 3.5f;
		static constexpr float BLOOM_INTENSITY = 1.0f;

		static constexpr float FOG_DISTANCE_FALLOFF_START = 0.5f;
		static constexpr float FOG_DISTANCE_FALLOFF_LENGTH = 5.0f;
		static constexpr float FOG_TILING = 0.12f;

		static constexpr float COLOR_GRADING_FACTOR = 0.25f;

		static constexpr float3 COLOR_GRADING = float3(1.0f, 1.1f, 1.4f);
		
		static constexpr uint32_t SPARKLES_NUMBER = 20u;

		struct MutableConstants
		{
		public:
			float4x4 world;
			float4x4 viewProjection;
			float3 cameraDirection;
			float time;
		};

		float3 cameraPosition;
		float3 cameraLookAt;
		float3 cameraUpVector;
		
		uint2 renderSize;

		float timer;
		float _deltaTime;
		float fps;
		uint32_t frameCounter;
		uint64_t cpuTimeCounter;

		std::chrono::steady_clock::time_point prevTimePoint;

		float3 environmentPosition;
		
		float3 windDirection;
		float windStrength;

		float4x4 environmentWorld;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID vfxAtlasId;
		Graphics::Resources::ResourceID perlinNoiseId;
		Graphics::Resources::ResourceID lightParticleBufferId;

		Graphics::Resources::ResourceID depthPassVSId;

		Graphics::Resources::ResourceID depthCubePassVSId;
		Graphics::Resources::ResourceID depthCubePassGSId;

		Graphics::Resources::ResourceID particleSimulationCSId;
		Graphics::Resources::ResourceID particleLightSimulationCSId;

		SceneEntity::Camera* camera;

		Graphics::Assets::Material* depthPassMaterial;
		Graphics::Assets::Material* depthCubePassMaterial;

		SceneEntity::LightID areaLightId;
		SceneEntity::LightID ambientLightId;

		SceneEntity::Terrain* terrain;
		SceneEntity::VegatationSystem* vegetationSystem;
		SceneEntity::LightingSystem* lightingSystem;

		SceneEntity::IDrawable* vfxLux;
		SceneEntity::IDrawable* vfxLuxSparkles;
		SceneEntity::IDrawable* vfxLuxDistorters;

		SceneEntity::PostProcessManager* postProcessManager;
	};
}
