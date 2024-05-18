#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class IndexBufferFactory : public IResourceFactory
	{
	public:
		IndexBufferFactory(BufferManager* bufferManager);
		~IndexBufferFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		IndexBufferFactory() = delete;

		BufferManager* _bufferManager;
	};
}
