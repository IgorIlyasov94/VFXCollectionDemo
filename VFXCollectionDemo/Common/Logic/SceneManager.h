#pragma once

#include "../../Includes.h"
#include "../../Graphics/DirectX12Renderer.h"
#include "Scene/IScene.h"

namespace Common::Logic
{
	class SceneManager final
	{
	public:
		SceneManager();
		~SceneManager();

		Scene::SceneID AddScene(Scene::IScene* newScene);
		Scene::IScene* GetScene(Scene::SceneID id);
		Scene::IScene* GetCurrentScene();

		void LoadScene(Scene::SceneID id, Graphics::DirectX12Renderer* renderer);
		void SwitchToNextScene(Graphics::DirectX12Renderer* renderer);

	private:
		SceneManager(const SceneManager&) = delete;
		SceneManager(SceneManager&&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager& operator=(SceneManager&&) = delete;

		Graphics::DirectX12Renderer* _renderer;
		std::vector<Scene::IScene*> scenes;
		Scene::SceneID currentScene;
	};
}
