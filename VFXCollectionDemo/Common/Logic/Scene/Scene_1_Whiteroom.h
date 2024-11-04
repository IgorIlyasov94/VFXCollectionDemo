#pragma once

#include "../../../Includes.h"

#include "../SceneEntity/IDrawable.h"

#include "../../../Graphics/Assets/Mesh.h"
#include "../../../Graphics/Assets/RaytracingObjectBuilder.h"

#include "../../../Common/Logic/SceneEntity/PostProcessManager.h"

#include "../SceneEntity/Camera.h"
#include "../SceneEntity/LightingSystem.h"

#include "IScene.h"

namespace Common::Logic::Scene
{
	class Scene_1_WhiteRoom : public IScene
	{
	public:
		Scene_1_WhiteRoom();
		~Scene_1_WhiteRoom() override;

		void Load(Graphics::DirectX12Renderer* renderer) override;
		void Unload(Graphics::DirectX12Renderer* renderer) override;

		void OnResize(Graphics::DirectX12Renderer* renderer) override;

		void Update() override;
		void RenderShadows(ID3D12GraphicsCommandList* commandList) override;
		void Render(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer) override;
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) override;

		bool IsLoaded() override;

	private:
		void LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateConstantBuffers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			uint32_t width, uint32_t height);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateLights(Graphics::DirectX12Renderer* renderer);

		void CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		void CreateObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Graphics::Resources::ResourceManager* resourceManager);
		
		void LoadMesh(std::filesystem::path filePath, std::filesystem::path fileCachePath,
			std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData, Graphics::Assets::MeshDesc& desc);

		void LoadCache(std::filesystem::path filePath, Graphics::Assets::MeshDesc& meshDesc,
			std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData);

		void SaveCache(std::filesystem::path filePath, const Graphics::Assets::MeshDesc& meshDesc,
			const std::vector<uint8_t>& verticesData, const std::vector<uint8_t>& indicesData);

		bool isLoaded;
		
		static constexpr float FOV_Y = DirectX::XM_PI / 2.5f;
		static constexpr float Z_NEAR = 0.1f;
		static constexpr float Z_FAR = 1000.0f;

		static constexpr bool DEPTH_PREPASS_ENABLED = false;
		static constexpr bool FSR_ENABLED = true;
		static constexpr bool MOTION_BLUR_ENABLED = false;
		static constexpr bool VOLUMETRIC_FOG_ENABLED = false;
		static constexpr bool USING_PARTICLE_LIGHT = false;

		static constexpr float WHITE_CUTOFF = 0.7f;
		static constexpr float BRIGHT_THRESHOLD = 3.5f;
		static constexpr float BLOOM_INTENSITY = 1.0f;

		static constexpr float COLOR_GRADING_FACTOR = 0.25f;

		static constexpr float3 COLOR_GRADING = float3(1.0f, 1.1f, 1.4f);

		static constexpr uint32_t MAX_SIMULTANEOUS_BARRIER_NUMBER = 2u;

		struct MutableConstants
		{
		public:
			float4x4 invViewProjection;
			float3 cameraPosition;
			float padding;
			float4x4 viewProjection;
			float4x4 lastViewProjection;
		};

		struct Payload
		{
		public:
			float3 color;
			float hitT;
			uint32_t recursionDepth;
		};

		struct PayloadShadow
		{
		public:
			float3 lightFactor;
		};

		struct BuiltInTriangleIntersectionAttributes
		{
			float2 barycentrics;
		};

		float3 cameraPosition;
		float3 cameraLookAt;
		float3 cameraUpVector;
		
		float timer;
		float _deltaTime;
		float fps;
		uint32_t frameCounter;
		uint64_t cpuTimeCounter;
		Graphics::Assets::AccelerationStructureDesc accelerationStructureDesc;
		Graphics::Assets::AccelerationStructureDesc accelerationStructureCrystalDesc;

		uint32_t groupsNumberX;
		uint32_t groupsNumberY;
		float2 renderSize;

		std::chrono::steady_clock::time_point prevTimePoint;

		float lightIntensity0;
		float lightIntensity1;

		MutableConstants* mutableConstantsBuffer;

		Graphics::Assets::RaytracingObject* lightingObject;

		Graphics::Assets::Material* texturedQuadMaterial;

		const Graphics::Assets::Mesh* quadMesh;

		SceneEntity::Camera* camera;
		SceneEntity::LightingSystem* lightingSystem;

		std::vector<D3D12_RESOURCE_BARRIER> barriers;

		Graphics::Resources::GPUResource* resultTargetResource;
		Graphics::Resources::GPUResource* depthVelocityTargetResource;
		Graphics::Resources::GPUResource* whiteroomVertexBufferResource;
		Graphics::Resources::GPUResource* whiteroomIndexBufferResource;
		Graphics::Resources::GPUResource* crystalVertexBufferResource;
		Graphics::Resources::GPUResource* crystalIndexBufferResource;

		Graphics::Resources::ResourceID resultTargetId;
		Graphics::Resources::ResourceID depthVelocityTargetId;

		Graphics::Resources::ResourceID mutableConstantsId;
		Graphics::Resources::ResourceID whiteroomAlbedoId;
		Graphics::Resources::ResourceID whiteroomNormalId;
		Graphics::Resources::ResourceID whiteroomVertexBufferId;
		Graphics::Resources::ResourceID whiteroomIndexBufferId;
		Graphics::Resources::ResourceID crystalVertexBufferId;
		Graphics::Resources::ResourceID crystalIndexBufferId;
		Graphics::Resources::ResourceID noiseId;

		Graphics::Resources::ResourceID lightingRSId;
		Graphics::Resources::ResourceID quadVSId;
		Graphics::Resources::ResourceID quadPSId;

		SceneEntity::LightID pointLight0Id;
		SceneEntity::LightID pointLight1Id;

		SceneEntity::PostProcessManager* postProcessManager;
	};
}
