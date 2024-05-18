#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class BufferFactory : public IResourceFactory
	{
	public:
		BufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager);
		~BufferFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		BufferFactory() = delete;

		BufferManager* _bufferManager;
		DescriptorManager* _descriptorManager;
	};
}
