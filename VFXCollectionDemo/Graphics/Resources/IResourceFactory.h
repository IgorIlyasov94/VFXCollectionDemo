#pragma once

#include "../DirectX12Includes.h"
#include "IResource.h"
#include "IResourceDesc.h"

namespace Graphics::Resources
{
	class IResourceFactory
	{
	public:
		virtual ~IResourceFactory() = 0 {};
		virtual IResource* CreateResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const IResourceDesc* desc) = 0;
	};
}
