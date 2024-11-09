#pragma once

#include "../../Resources/ResourceManager.h"

namespace Graphics::Assets::Loaders
{
	class ResourceLoadingQueue final
	{
	public:
		ResourceLoadingQueue();
		~ResourceLoadingQueue();

		void LoadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const std::filesystem::path& filePath,
			Graphics::Resources::ResourceID* loadedResourceId);

		void Complete();

	private:
		ResourceLoadingQueue(const ResourceLoadingQueue&) = delete;
		ResourceLoadingQueue(ResourceLoadingQueue&&) = delete;
		ResourceLoadingQueue& operator=(const ResourceLoadingQueue&) = delete;
		ResourceLoadingQueue& operator=(ResourceLoadingQueue&&) = delete;

		static void LoadTextureFunc(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Graphics::Resources::ResourceManager* resourceManager, const std::filesystem::path& filePath,
			Graphics::Resources::ResourceID* loadedResourceId);

		uint32_t threadsMaxNumber;
		uint32_t threadsNumber;
		uint32_t currentThreadIndex;

		std::vector<std::thread> threads;
	};
}
