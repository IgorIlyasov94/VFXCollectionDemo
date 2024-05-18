#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../BufferManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class ConstantBufferFactory : public IResourceFactory
	{
	public:
		ConstantBufferFactory(BufferManager* bufferManager, DescriptorManager* descriptorManager);
		~ConstantBufferFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		ConstantBufferFactory() = delete;

		BufferManager* _bufferManager;
		DescriptorManager* _descriptorManager;
	};
}
