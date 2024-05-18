#pragma once

#include "../DirectX12Includes.h"
#include "../DescriptorManager.h"
#include "../TextureManager.h"
#include "IResourceFactory.h"

namespace Graphics::Resources
{
	class RWTextureFactory : public IResourceFactory
	{
	public:
		RWTextureFactory(TextureManager* textureManager, DescriptorManager* descriptorManager);
		~RWTextureFactory();

		IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) override;

	private:
		RWTextureFactory() = delete;

		TextureManager* _textureManager;
		DescriptorManager* _descriptorManager;
	};
}
