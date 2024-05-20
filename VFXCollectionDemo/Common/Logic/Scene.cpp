#include "Scene.h"

Common::Logic::Scene::Scene()
	: isLoaded(false)
{

}

Common::Logic::Scene::~Scene()
{

}

void Common::Logic::Scene::Load()
{
	isLoaded = true;
}

void Common::Logic::Scene::Unload()
{
	isLoaded = false;
}

void Common::Logic::Scene::Update()
{

}

void Common::Logic::Scene::Render(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::Scene::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{

}

bool Common::Logic::Scene::IsLoaded()
{
	return isLoaded;
}
