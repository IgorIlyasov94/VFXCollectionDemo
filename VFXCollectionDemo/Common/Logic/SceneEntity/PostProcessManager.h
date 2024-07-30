#pragma once

#include "../../../Includes.h"

#include "../../../Graphics/DirectX12Renderer.h"
#include "../../../Graphics/Assets/Material.h"
#include "../../../Graphics/Assets/ComputeObject.h"
#include "../../../Graphics/Assets/Mesh.h"

namespace Common::Logic::SceneEntity
{
	class PostProcessManager
	{
	public:
		PostProcessManager(ID3D12GraphicsCommandList* commandList, Graphics::DirectX12Renderer* renderer);
		~PostProcessManager();

		void Release(Graphics::Resources::ResourceManager* resourceManager);

		void OnResize(Graphics::DirectX12Renderer* renderer);

		void SetDepthPrepass(ID3D12GraphicsCommandList* commandList);
		void SetGBuffer(ID3D12GraphicsCommandList* commandList);

		void SetDistortBuffer(ID3D12GraphicsCommandList* commandList);

		void Render(ID3D12GraphicsCommandList* commandList);
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList);

	private:
		PostProcessManager() = delete;

		void CreateQuad(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager);

		void CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height);

		void LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);
		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::DirectX12Renderer* renderer);
		void CreateComputeObjects(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		struct QuadVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
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
		};

		static constexpr float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

		static constexpr float MIDDLE_GRAY = 0.15f;
		static constexpr float WHITE_CUTOFF = 0.75f;
		static constexpr float BRIGHT_THRESHOLD = 4.0f;
		static constexpr float BLOOM_INTENSITY = 4.0f;

		static constexpr uint32_t THREADS_PER_GROUP = 64u;
		static constexpr uint32_t HALF_BLUR_SAMPLES_NUMBER = 8u;
		static constexpr uint32_t MAX_SIMULTANEOUS_BARRIER_NUMBER = 5u;

		MotionBlurConstants motionBlurConstants;
		HDRConstants hdrConstants;
		ToneMappingConstants toneMappingConstants;

		uint32_t _width;
		uint32_t _height;

		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRectangle;

		D3D12_CPU_DESCRIPTOR_HANDLE sceneColorTargetDescriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE sceneDepthTargetDescriptor;

		D3D12_CPU_DESCRIPTOR_HANDLE sceneMotionTargetDescriptor;

		std::vector<D3D12_RESOURCE_BARRIER> barriers;

		Graphics::Resources::GPUResource* sceneColorTargetGPUResource;
		Graphics::Resources::GPUResource* sceneMotionTargetGPUResource;
		Graphics::Resources::GPUResource* sceneBufferGPUResource;
		Graphics::Resources::GPUResource* luminanceBufferGPUResource;
		Graphics::Resources::GPUResource* bloomBufferGPUResource;

		Graphics::Resources::ResourceID sceneColorTargetId;
		Graphics::Resources::ResourceID sceneDepthTargetId;

		Graphics::Resources::ResourceID sceneMotionTargetId;
		Graphics::Resources::ResourceID sceneBufferId;
		Graphics::Resources::ResourceID luminanceBufferId;
		Graphics::Resources::ResourceID bloomBufferId;

		Graphics::Resources::ResourceID quadVSId;
		Graphics::Resources::ResourceID motionBlurCSId;
		Graphics::Resources::ResourceID luminanceCSId;
		Graphics::Resources::ResourceID luminanceIterationCSId;
		Graphics::Resources::ResourceID bloomHorizontalCSId;
		Graphics::Resources::ResourceID bloomVerticalCSId;
		Graphics::Resources::ResourceID toneMappingPSId;

		Graphics::Assets::Mesh* quadMesh;
		Graphics::Assets::ComputeObject* motionBlurComputeObject;
		Graphics::Assets::ComputeObject* luminanceComputeObject;
		Graphics::Assets::ComputeObject* luminanceIterationComputeObject;
		Graphics::Assets::ComputeObject* bloomHorizontalObject;
		Graphics::Assets::ComputeObject* bloomVerticalObject;
		Graphics::Assets::Material* toneMappingMaterial;
	};
}
