#include "SceneManager.h"

Common::SceneManager::SceneManager()
	: _renderer(nullptr)
{

}

Common::SceneManager::~SceneManager()
{

}

void Common::SceneManager::SetRenderer(Graphics::DirectX12Renderer* renderer)
{
	_renderer = renderer;
}

void Common::SceneManager::OnResize(uint32_t newWidth, uint32_t newHeight)
{
	if (_renderer != nullptr)
		_renderer->OnResize(newWidth, newHeight);
}

void Common::SceneManager::OnSetFocus()
{
	if (_renderer != nullptr)
		_renderer->OnSetFocus();
}

void Common::SceneManager::OnLostFocus()
{
	if (_renderer != nullptr)
		_renderer->OnLostFocus();
}

void Common::SceneManager::OnDeviceLost()
{
	if (_renderer != nullptr)
		_renderer->OnDeviceLost();
}

void Common::SceneManager::OnFrameRender()
{
	if (_renderer == nullptr)
		return;

	_renderer->FrameStart();
	_renderer->FrameEnd();
}

void Common::SceneManager::SwitchFullscreenMode(bool enableFullscreen)
{
	if (_renderer != nullptr)
		_renderer->SwitchFullscreenMode(enableFullscreen);
}
