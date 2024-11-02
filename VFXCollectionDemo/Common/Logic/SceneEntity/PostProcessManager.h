#pragma once

#include "../../../Includes.h"
#include "Camera.h"
#include "LightingSystem.h"
#include "FSR.h"
#include "../../../Graphics/DirectX12Renderer.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/ComputeObject.h"
#include "../../../Graphics/Assets/Mesh.h"

namespace Common::Logic::SceneEntity
{
	struct RenderingScheme
	{
		float whiteCutoff;
		float brightThreshold;
		float bloomIntensity;
		
		float3 colorGrading;
		float colorGradingFactor;

		float fogDistanceFalloffStart;
		float fogDistanceFalloffLength;
		float fogTiling;

		float3* windDirection;
		float* windStrength;

		bool enableDepthPrepass;
		bool enableMotionBlur;
		bool enableVolumetricFog;
		bool enableFSR;
		bool useParticleLight;
	};

	class PostProcessManager
	{
	public:
		PostProcessManager(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer,
			Camera* camera, LightingSystem* lightingSystem, const std::vector<DxcDefine>& lightDefines,
			const RenderingScheme& renderingScheme);
		~PostProcessManager();

		void Release(Graphics::Resources::ResourceManager* resourceManager);

		void OnResize(Graphics::DirectX12Renderer* renderer);

		void SetDepthPrepass(ID3D12GraphicsCommandList* commandList);
		void SetGBuffer(ID3D12GraphicsCommandList* commandList);

		void SetMotionBuffer(ID3D12GraphicsCommandList* commandList);

		void Render(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer, float deltaTime);
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList);

		const Graphics::Assets::Mesh* GetFullscreenQuadMesh() const;

	private:
		PostProcessManager() = delete;

		void CreateConstantBuffers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			uint32_t width, uint32_t height);

		void CreateQuad(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			const std::vector<DxcDefine>& lightDefines);

		void CreateTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::DirectX12Renderer* renderer);
		void CreateComputeObjects(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			LightingSystem* lightingSystem);

		void UpdateObjects(Graphics::DirectX12Renderer* renderer);

		void SetMotionBlur(ID3D12GraphicsCommandList* commandList);
		void SetVolumetricFog(ID3D12GraphicsCommandList* commandList);
		void SetFSR(ID3D12GraphicsCommandList* commandList, float deltaTime);
		void SetHDR(ID3D12GraphicsCommandList* commandList);

		struct QuadVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
		};

		struct VolumetricFogConstants
		{
		public:
			float4x4 invViewProjection;

			uint32_t widthU;
			uint32_t area;
			float width;
			float height;

			float3 cameraDirection;
			float fogTiling;

			float3 cameraPosition;
			float distanceFalloffStart;

			float3 fogOffset;
			float distanceFalloffLength;

			float zNear;
			float zFar;
			float2 padding;
		};

		struct MotionBlurConstants
		{
		public:
			uint32_t widthU;
			uint32_t area;
			float width;
			float height;
		};

		struct HDRConstants
		{
		public:
			uint32_t width;
			uint32_t area;
			float middleGray;
			float whiteCutoff;
			float brightThreshold;
		};

		struct ToneMappingConstants
		{
		public:
			uint32_t width;
			uint32_t area;
			uint32_t halfWidth;
			uint32_t quartArea;
			float middleGray;
			float whiteCutoff;
			float brightThreshold;
			float bloomIntensity;
			float3 colorGrading;
			float colorGradingFactor;
		};

		static constexpr float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		static constexpr float FOG_MOVING_COEFF = 0.002f;
		static constexpr float SHARPNESS_FACTOR = 1.0f;

		static constexpr uint32_t THREADS_PER_GROUP = 64u;
		static constexpr uint32_t HALF_BLUR_SAMPLES_NUMBER = 8u;
		static constexpr uint32_t MAX_SIMULTANEOUS_BARRIER_NUMBER = 5u;

		static constexpr uint32_t NOISE_SIZE_X = 128u;
		static constexpr uint32_t NOISE_SIZE_Y = 128u;
		static constexpr uint32_t NOISE_SIZE_Z = 64u;

		static constexpr uint32_t FSR_SIZE_NUMERATOR = 1u;
		static constexpr uint32_t FSR_SIZE_DENOMINATOR = 2u;

		VolumetricFogConstants* volumetricFogConstants;
		MotionBlurConstants motionBlurConstants;
		HDRConstants hdrConstants;
		ToneMappingConstants toneMappingConstants;

		Camera* _camera;
		FSR* _fsr;

		uint32_t _width;
		uint32_t _height;

		D3D12_VIEWPORT viewport;
		D3D12_VIEWPORT afterFSRViewport;
		D3D12_RECT scissorRectangle;
		D3D12_RECT afterFSRScissorRectangle;

		D3D12_CPU_DESCRIPTOR_HANDLE sceneColorTargetDescriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE sceneDepthTargetDescriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE sceneAlphaTargetDescriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE sceneMotionTargetDescriptor;

		RenderingScheme _renderingScheme;

		std::vector<D3D12_RESOURCE_BARRIER> barriers;

		Graphics::Resources::GPUResource* sceneColorTargetGPUResource;
		Graphics::Resources::GPUResource* sceneDepthTargetGPUResource;
		Graphics::Resources::GPUResource* sceneAlphaTargetGPUResource;
		Graphics::Resources::GPUResource* sceneMotionTargetGPUResource;
		Graphics::Resources::GPUResource* temporaryRWTextureGPUResource;
		Graphics::Resources::GPUResource* luminanceBufferGPUResource;
		Graphics::Resources::GPUResource* bloomBufferGPUResource;

		Graphics::Resources::ResourceID sceneColorTargetId;
		Graphics::Resources::ResourceID sceneDepthTargetId;
		Graphics::Resources::ResourceID sceneAlphaTargetId;

		Graphics::Resources::ResourceID volumetricFogConstantBufferId;
		Graphics::Resources::ResourceID sceneMotionTargetId;
		Graphics::Resources::ResourceID temporaryRWTextureId;
		Graphics::Resources::ResourceID luminanceBufferId;
		Graphics::Resources::ResourceID bloomBufferId;

		Graphics::Resources::ResourceID fogMapId;
		Graphics::Resources::ResourceID turbulenceMapId;

		Graphics::Resources::ResourceID quadVSId;
		Graphics::Resources::ResourceID motionBlurCSId;
		Graphics::Resources::ResourceID volumetricFogCSId;
		Graphics::Resources::ResourceID luminanceCSId;
		Graphics::Resources::ResourceID luminanceIterationCSId;
		Graphics::Resources::ResourceID bloomHorizontalCSId;
		Graphics::Resources::ResourceID bloomVerticalCSId;
		Graphics::Resources::ResourceID toneMappingPSId;
		Graphics::Resources::ResourceID copyAlphaPSId;

		Graphics::Assets::Mesh* quadMesh;
		Graphics::Assets::ComputeObject* motionBlurComputeObject;
		Graphics::Assets::ComputeObject* volumetricFogComputeObject;
		Graphics::Assets::ComputeObject* luminanceComputeObject;
		Graphics::Assets::ComputeObject* luminanceIterationComputeObject;
		Graphics::Assets::ComputeObject* bloomHorizontalObject;
		Graphics::Assets::ComputeObject* bloomVerticalObject;
		Graphics::Assets::Material* toneMappingMaterial;
		Graphics::Assets::Material* copyAlphaMaterial;
	};
}
