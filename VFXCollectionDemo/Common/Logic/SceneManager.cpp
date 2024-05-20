#include "SceneManager.h"

Common::Logic::SceneManager::SceneManager()
	: currentScene(-1), emptyScene{}
{

}

Common::Logic::SceneManager::~SceneManager()
{

}

Common::Logic::SceneID Common::Logic::SceneManager::AddScene()
{
	auto sceneId = static_cast<SceneID>(scenes.size());

	scenes.push_back({});

	return sceneId;
}

Common::Logic::Scene& Common::Logic::SceneManager::GetScene(SceneID id)
{
	if (id < 0u || id >= scenes.size())
		return emptyScene;

	return scenes[id];
}

Common::Logic::Scene& Common::Logic::SceneManager::GetCurrentScene()
{
	if (currentScene < 0u || currentScene >= scenes.size())
		return emptyScene;

	return scenes[currentScene];
}

void Common::Logic::SceneManager::LoadScene(SceneID id)
{
	if (scenes[id].IsLoaded())
		return;

	scenes[currentScene].Unload();
	scenes[id].Load();

	currentScene = id;
}
