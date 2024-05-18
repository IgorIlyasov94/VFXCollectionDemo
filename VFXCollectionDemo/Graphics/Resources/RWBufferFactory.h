#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class RWBufferFactory : public IResourceFactory
	{
	public:
		RWBufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager);
		~RWBufferFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		RWBufferFactory() = delete;

		BufferManager* _bufferManager;
		DescriptorManager* _descriptorManager;
	};
}
