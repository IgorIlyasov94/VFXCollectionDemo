#pragma once

#include "../../../Includes.h"

#include "../SceneEntity/IDrawable.h"

#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/Assets/Material.h"

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

		void OnResize(uint32_t newWidth, uint32_t newHeight) override;

		void Update() override;
		void Render(ID3D12GraphicsCommandList* commandList) override;
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) override;

		bool IsLoaded() override;

	private:
		bool isLoaded;
		
		static constexpr float FOV_Y = DirectX::XM_PI / 4.0f;
		static constexpr float Z_NEAR = 0.01f;
		static constexpr float Z_FAR = 1000.0f;

		struct MutableConstants
		{
		public:
			float4x4 worldViewProjection;
		};

		float3 cameraPosition;
		float3 cameraLookAt;
		float3 cameraUpVector;
		
		float timer;

		float3 environmentPosition;

		float4x4 environmentWorld;

		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRectangle;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID samplerLinearId;
		Graphics::Resources::ResourceID environmentFloorAlbedoId;
		Graphics::Resources::ResourceID environmentFloorNormalId;

		Graphics::Resources::ResourceID pbrStandardVSId;
		Graphics::Resources::ResourceID pbrStandardPSId;

		SceneEntity::Camera* camera;

		Graphics::Assets::Material* environmentMaterial;
		Graphics::Assets::Mesh* environmentMesh;
		SceneEntity::IDrawable* environmentMeshObject;
	};
}
