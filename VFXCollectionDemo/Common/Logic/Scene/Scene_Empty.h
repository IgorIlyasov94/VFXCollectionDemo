#pragma once

#include "../../../Includes.h"
#include "IScene.h"

namespace Common::Logic::Scene
{
	class Scene_Empty : public IScene
	{
	public:
		Scene_Empty();
		~Scene_Empty() override;

		void Load(Graphics::DirectX12Renderer* renderer) override;
		void Unload(Graphics::DirectX12Renderer* renderer) override;

		void OnResize(Graphics::DirectX12Renderer* renderer) override;

		void Update() override;
		void Render(ID3D12GraphicsCommandList* commandList) override;
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList) override;

		bool IsLoaded() override;
	};
}
