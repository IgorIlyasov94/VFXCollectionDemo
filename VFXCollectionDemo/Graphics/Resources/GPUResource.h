#pragma once

#include "../DirectX12Includes.h"

namespace Graphics::Resources
{
	class GPUResource
	{
	public:
		GPUResource(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _currentState);
		~GPUResource();

		ID3D12Resource* GetResource();

		uint8_t* Map();
		void Unmap();

		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState);
		void BeginBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState);
		void EndBarrier(ID3D12GraphicsCommandList* commandList);
		void UAVBarrier(ID3D12GraphicsCommandList* commandList);

		D3D12_RESOURCE_BARRIER GetBarrier(D3D12_RESOURCE_STATES newState);
		D3D12_RESOURCE_BARRIER GetBeginBarrier(D3D12_RESOURCE_STATES newState);
		D3D12_RESOURCE_BARRIER GetEndBarrier();
		D3D12_RESOURCE_BARRIER GetUAVBarrier();

	private:
		GPUResource() = delete;
		GPUResource(const GPUResource&) = delete;
		GPUResource(GPUResource&&) = delete;
		GPUResource& operator=(const GPUResource&) = delete;
		GPUResource& operator=(GPUResource&&) = delete;

		ID3D12Resource* resource;
		D3D12_RESOURCE_STATES currentState;
		D3D12_RESOURCE_BARRIER barrier;
	};
}
