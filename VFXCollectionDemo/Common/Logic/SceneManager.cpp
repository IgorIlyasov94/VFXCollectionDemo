#include "SceneManager.h"
#include "Scene/Scene_Empty.h"

Common::Logic::SceneManager::SceneManager()
	: currentScene(0u), _renderer(nullptr)
{
	scenes.push_back(new Scene::Scene_Empty());
}

Common::Logic::SceneManager::~SceneManager()
{
	for (auto& scene : scenes)
	{
		if (scene->IsLoaded())
			scene->Unload(_renderer);

		delete scene;
	}
}

Common::Logic::Scene::SceneID Common::Logic::SceneManager::AddScene(Scene::IScene* newScene)
{
	auto sceneId = static_cast<Scene::SceneID>(scenes.size());

	scenes.push_back(newScene);

	return sceneId;
}

Common::Logic::Scene::IScene* Common::Logic::SceneManager::GetScene(Scene::SceneID id)
{
	return scenes[id];
}

Common::Logic::Scene::IScene* Common::Logic::SceneManager::GetCurrentScene()
{
	return scenes[currentScene];
}

void Common::Logic::SceneManager::LoadScene(Scene::SceneID id, Graphics::DirectX12Renderer* renderer)
{
	if (scenes[id]->IsLoaded())
		return;

	_renderer = renderer;

	scenes[currentScene]->Unload(renderer);
	scenes[id]->Load(renderer);

	currentScene = id;
}
