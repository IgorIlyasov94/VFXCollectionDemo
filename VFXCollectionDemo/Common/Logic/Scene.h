#pragma once

#include "../../Includes.h"
#include "../../Graphics/DirectX12Includes.h"
#include "IDrawable.h"

namespace Common::Logic
{
	using SceneID = uint32_t;

	class Scene
	{
	public:
		Scene();
		~Scene();

		void Load();
		void Unload();

		void Update();
		void Render(ID3D12GraphicsCommandList* commandList);
		void RenderToBackBuffer(ID3D12GraphicsCommandList* commandList);

		bool IsLoaded();

	private:
		bool isLoaded;

		std::vector<IDrawable*> drawableObjects;
	};
}
