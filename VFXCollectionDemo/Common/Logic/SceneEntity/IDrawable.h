#pragma once

#include "../../../Includes.h"
#include "../../../Graphics/DirectX12Includes.h"
#include "../../../Graphics/Resources/ResourceManager.h"
#include "../../../Graphics/VertexFormat.h"

namespace Common::Logic::SceneEntity
{
	class IDrawable
	{
	public:
		virtual ~IDrawable() = 0 {};

		virtual void Update(float time, float deltaTime) = 0;

		virtual void OnCompute(ID3D12GraphicsCommandList* commandList) = 0;
		
		virtual void DrawDepthPrepass(ID3D12GraphicsCommandList* commandList) = 0;
		virtual void DrawShadows(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) = 0;
		virtual void DrawShadowsCube(ID3D12GraphicsCommandList* commandList, uint32_t lightMatrixStartIndex) = 0;
		virtual void Draw(ID3D12GraphicsCommandList* commandList) = 0;

		virtual void Release(Graphics::Resources::ResourceManager* resourceManager) = 0;
	};
}
