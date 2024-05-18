#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../TextureManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class DepthStencilFactory : public IResourceFactory
	{
	public:
		DepthStencilFactory(TextureManager* textureManager, DescriptorManager* descriptorManager);
		~DepthStencilFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		DepthStencilFactory() = delete;

		TextureManager* _textureManager;
		DescriptorManager* _descriptorManager;
	};
}
