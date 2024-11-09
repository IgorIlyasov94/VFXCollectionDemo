#include "ResourceLoadingQueue.h"
#include "DDSLoader.h"

using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Graphics::Assets::Loaders::ResourceLoadingQueue::ResourceLoadingQueue()
{
	threadsNumber = 0u;
	currentThreadIndex = 0u;
	threadsMaxNumber = std::thread::hardware_concurrency();
	threads.resize(threadsMaxNumber);
}

Graphics::Assets::Loaders::ResourceLoadingQueue::~ResourceLoadingQueue()
{
	Complete();
}

void Graphics::Assets::Loaders::ResourceLoadingQueue::LoadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager, const std::filesystem::path& filePath, ResourceID* loadedResourceId)
{
	if ((threadsNumber++) >= threadsMaxNumber)
	{
		if (threads[currentThreadIndex].joinable())
			threads[currentThreadIndex].join();
	}

	threads[currentThreadIndex] = std::thread(&ResourceLoadingQueue::LoadTextureFunc,
		device, commandList, resourceManager, filePath, loadedResourceId);

	currentThreadIndex++;

	if (currentThreadIndex >= threadsMaxNumber)
		currentThreadIndex = 0u;
}

void Graphics::Assets::Loaders::ResourceLoadingQueue::Complete()
{
	for (uint32_t threadIndex = 0u; threadIndex < threadsNumber; threadIndex++)
		if (threads[threadIndex].joinable())
			threads[threadIndex].join();
}

void Graphics::Assets::Loaders::ResourceLoadingQueue::LoadTextureFunc(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager, const std::filesystem::path& filePath, ResourceID* loadedResourceId)
{
	TextureDesc textureDesc{};
	DDSLoader::Load(filePath, textureDesc);
	*loadedResourceId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}
