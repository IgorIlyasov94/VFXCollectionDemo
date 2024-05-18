#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../TextureManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class RenderTargetFactory : public IResourceFactory
	{
	public:
		RenderTargetFactory(TextureManager* textureManager, DescriptorManager* descriptorManager);
		~RenderTargetFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		RenderTargetFactory() = delete;

		TextureManager* _textureManager;
		DescriptorManager* _descriptorManager;
	};
}
