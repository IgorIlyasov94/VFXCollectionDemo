#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"

namespace Common::Logic::SceneEntity
{
	class IDrawable
	{
	public:
		virtual ~IDrawable() = 0 {};

		virtual void Draw(ID3D12GraphicsCommandList* commandList) = 0;
	};
}
