#pragma once

#include "../../../Includes.h"

#include "../SceneEntity/IDrawable.h"

#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/Assets/Material.h"

#include "../../../Common/Logic/SceneEntity/PostProcessManager.h"

#include "../SceneEntity/Camera.h"

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
		void Render(ID3D12GraphicsCommandList* commandList) override;
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) override;

		bool IsLoaded() override;

	private:
		void LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateSamplers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);
		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::DirectX12Renderer* renderer);
		void CreateObjects();

		bool isLoaded;
		
		static constexpr float FOV_Y = DirectX::XM_PI / 4.0f;
		static constexpr float Z_NEAR = 0.01f;
		static constexpr float Z_FAR = 1000.0f;

		struct MutableConstants
		{
		public:
			float4x4 world;
			float4x4 viewProjection;
			float4 cameraDirection;
		};

		float3 cameraPosition;
		float3 cameraLookAt;
		float3 cameraUpVector;
		
		float timer;
		float fps;
		uint32_t frameCounter;
		uint64_t cpuTimeCounter;

		std::chrono::steady_clock::time_point prevTimePoint;

		float3 environmentPosition;

		float4x4 environmentWorld;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID samplerLinearId;
		Graphics::Resources::ResourceID environmentFloorAlbedoId;
		Graphics::Resources::ResourceID environmentFloorNormalId;

		Graphics::Resources::ResourceID vfxAtlasId;

		Graphics::Resources::ResourceID pbrStandardVSId;
		Graphics::Resources::ResourceID pbrStandardPSId;

		SceneEntity::Camera* camera;

		Graphics::Assets::Material* environmentMaterial;
		Graphics::Assets::Mesh* environmentMesh;
		SceneEntity::IDrawable* environmentMeshObject;

		SceneEntity::IDrawable* vfxLux;

		SceneEntity::PostProcessManager* postProcessManager;
	};
}
