#include "GPUResource.h"

Graphics::Resources::GPUResource::GPUResource(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _currentState)
	: resource(_resource), currentState(_currentState), barrier{}
{
	barrier.Transition.pResource = _resource;
}

Graphics::Resources::GPUResource::~GPUResource()
{
	resource->Release();
}

ID3D12Resource* Graphics::Resources::GPUResource::GetResource()
{
	return resource;
}

uint8_t* Graphics::Resources::GPUResource::Map()
{
	uint8_t* cpuAddress = nullptr;

	D3D12_RANGE range{ 0u, 0u };

	resource->Map(0u, &range, reinterpret_cast<void**>(&cpuAddress));

	return cpuAddress;
}

void Graphics::Resources::GPUResource::Unmap()
{
	resource->Unmap(0u, nullptr);
}

void Graphics::Resources::GPUResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState)
{
	if (currentState == newState)
		return;

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;

	currentState = newState;

	commandList->ResourceBarrier(1u, &barrier);
}

void Graphics::Resources::GPUResource::BeginBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState)
{
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;

	if (currentState == newState)
		return;

	currentState = newState;

	commandList->ResourceBarrier(1u, &barrier);
}

void Graphics::Resources::GPUResource::EndBarrier(ID3D12GraphicsCommandList* commandList)
{
	if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
		return;

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

	commandList->ResourceBarrier(1u, &barrier);
}

void Graphics::Resources::GPUResource::UAVBarrier(ID3D12GraphicsCommandList* commandList)
{
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	commandList->ResourceBarrier(1u, &barrier);
}

D3D12_RESOURCE_BARRIER Graphics::Resources::GPUResource::GetBarrier(D3D12_RESOURCE_STATES newState)
{
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;

	currentState = newState;

	return barrier;
}

D3D12_RESOURCE_BARRIER Graphics::Resources::GPUResource::GetBeginBarrier(D3D12_RESOURCE_STATES newState)
{
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;

	currentState = newState;

	return barrier;
}

D3D12_RESOURCE_BARRIER Graphics::Resources::GPUResource::GetEndBarrier()
{
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

	return barrier;
}

D3D12_RESOURCE_BARRIER Graphics::Resources::GPUResource::GetUAVBarrier()
{
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	return barrier;
}