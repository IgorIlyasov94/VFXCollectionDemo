#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"
#include "../../../Graphics/Resources/ResourceManager.h"

namespace Common::Logic::SceneEntity
{
	class IDrawable
	{
	public:
		virtual ~IDrawable() = 0 {};

		virtual void OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) = 0;
		virtual void Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime) = 0;

		virtual void Release(Graphics::Resources::ResourceManager* resourceManager) = 0;
	};
}
