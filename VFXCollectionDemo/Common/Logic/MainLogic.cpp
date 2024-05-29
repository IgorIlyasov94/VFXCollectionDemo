#include "MainLogic.h"
#include "Scene/Scene_0_Lux.h"

Common::Logic::MainLogic::MainLogic(const RECT& windowPlacement, HWND windowHandler, bool isFullscreen)
	: _isFullscreen(isFullscreen), needBackgroundUpdate(true)
{
	renderer = new Graphics::DirectX12Renderer(windowPlacement, windowHandler, isFullscreen);
	sceneManager = new SceneManager();

	luxSceneId = sceneManager->AddScene(new Scene::Scene_0_Lux());

	sceneManager->LoadScene(luxSceneId, renderer);
}

Common::Logic::MainLogic::~MainLogic()
{
	renderer->FlushQueue();

	delete sceneManager;
	delete renderer;
}

void Common::Logic::MainLogic::OnResize(uint32_t newWidth, uint32_t newHeight, HWND windowHandler)
{
	renderer->OnResize(newWidth, newHeight, windowHandler);

	auto scene = sceneManager->GetCurrentScene();
	scene->OnResize(renderer);
}

void Common::Logic::MainLogic::OnSetFocus(HWND windowHandler)
{
	renderer->OnSetFocus(windowHandler);
}

void Common::Logic::MainLogic::OnLostFocus(HWND windowHandler)
{
	renderer->OnLostFocus(windowHandler);
}

void Common::Logic::MainLogic::Update()
{
	auto scene = sceneManager->GetCurrentScene();
	scene->Update();
}

void Common::Logic::MainLogic::Render()
{
	auto scene = sceneManager->GetCurrentScene();
	
	auto commandList = renderer->StartFrame();
	scene->Render(commandList);

	renderer->SetRenderToBackBuffer(commandList);
	scene->RenderToBackBuffer(commandList);

	renderer->EndFrame(commandList);
}

bool Common::Logic::MainLogic::NeedBackgroundUpdate()
{
	return needBackgroundUpdate;
}
