#pragma once

#include "../../Includes.h"
#include "Scene.h"

namespace Common::Logic
{
	class SceneManager final
	{
	public:
		SceneManager();
		~SceneManager();

		SceneID AddScene();
		Scene& GetScene(SceneID id);
		Scene& GetCurrentScene();

		void LoadScene(SceneID id);

	private:
		SceneManager(const SceneManager&) = delete;
		SceneManager(SceneManager&&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager& operator=(SceneManager&&) = delete;

		std::vector<Scene> scenes;
		Scene emptyScene;
		SceneID currentScene;
	};
}
