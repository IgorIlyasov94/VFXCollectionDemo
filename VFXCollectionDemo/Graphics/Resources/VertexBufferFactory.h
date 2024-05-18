#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class VertexBufferFactory : public IResourceFactory
	{
	public:
		VertexBufferFactory(BufferManager* bufferManager);
		~VertexBufferFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		VertexBufferFactory() = delete;
		
		BufferManager* _bufferManager;
	};
}
