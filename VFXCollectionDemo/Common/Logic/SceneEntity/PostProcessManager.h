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

		void SetGBuffer(ID3D12GraphicsCommandList* commandList);

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
		void CreateSamplers(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);
		void CreateMaterials(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager,
			Graphics::DirectX12Renderer* renderer);
		void CreateComputeObjects(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager);

		struct QuadVertex
		{
		public:
			float3 position;
			DirectX::PackedVector::XMHALF2 texCoord;
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

		static constexpr float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		static constexpr uint32_t THREADS_PER_GROUP = 64u;
		static constexpr uint32_t HALF_BLUR_SAMPLES_NUMBER = 8u;

		HDRConstants hdrConstants;

		uint32_t _width;
		uint32_t _height;

		D3D12_VIEWPORT viewport;
		D3D12_RECT scissorRectangle;

		D3D12_CPU_DESCRIPTOR_HANDLE sceneColorTargetDescriptor;
		D3D12_CPU_DESCRIPTOR_HANDLE sceneDepthTargetDescriptor;

		Graphics::Resources::GPUResource* sceneColorTargetGPUResource;
		Graphics::Resources::GPUResource* luminanceBufferGPUResource;
		Graphics::Resources::GPUResource* bloomBufferGPUResource;

		Graphics::Resources::ResourceID sceneColorTargetId;
		Graphics::Resources::ResourceID sceneDepthTargetId;

		Graphics::Resources::ResourceID luminanceBufferId;
		Graphics::Resources::ResourceID bloomBufferId;

		Graphics::Resources::ResourceID quadVSId;
		Graphics::Resources::ResourceID luminanceCSId;
		Graphics::Resources::ResourceID luminanceIterationCSId;
		Graphics::Resources::ResourceID bloomHorizontalCSId;
		Graphics::Resources::ResourceID bloomVerticalCSId;
		Graphics::Resources::ResourceID toneMappingPSId;

		Graphics::Resources::ResourceID samplerPointId;

		Graphics::Assets::Mesh* quadMesh;
		Graphics::Assets::ComputeObject* luminanceComputeObject;
		Graphics::Assets::ComputeObject* luminanceIterationComputeObject;
		Graphics::Assets::ComputeObject* bloomHorizontalObject;
		Graphics::Assets::ComputeObject* bloomVerticalObject;
		Graphics::Assets::Material* toneMappingMaterial;
	};
}