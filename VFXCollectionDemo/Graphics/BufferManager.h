#pragma once

#include "DirectX12Includes.h"
#include "Resources/GPUResource.h"

namespace Graphics
{
	enum class BufferAllocationType : uint32_t
	{
		COMMON = 0u,
		UNORDERED_ACCESS = 1u,
		VERTEX_CONSTANT = 2u,
		DYNAMIC_CONSTANT = 3u,
		INDEX = 4u,
		UPLOAD = 5u,
		SHADER_TABLES = 6u,
		ACCELERATION_STRUCTURE = 7u,
		UNORDERED_ACCESS_TEMP = 8u
	};

	struct BufferAllocation
	{
	public:
		Resources::GPUResource* resource;
		uint8_t* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
		uint64_t resourceOffset;
		uint64_t size;
	};

	class BufferManager
	{
	public:
		BufferManager();
		~BufferManager();

		BufferAllocation Allocate(ID3D12Device* device, uint64_t size, BufferAllocationType type);
		
		void Deallocate(Resources::GPUResource* allocatedResource, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, uint64_t size);

		void ReleaseTempBuffers();

	private:
		BufferManager(const BufferManager&) = delete;
		BufferManager(BufferManager&&) = delete;
		BufferManager& operator=(const BufferManager&) = delete;
		BufferManager& operator=(BufferManager&&) = delete;

		constexpr uint64_t AlignValue(uint64_t value, uint64_t alignment);

		struct BufferChunk
		{
			uint64_t offset;
			uint64_t size;
		};

		struct BufferSpace
		{
		public:
			Resources::GPUResource* resource;
			uint8_t* cpuStartAddress;
			D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress;
			BufferChunk chunk;
			BufferAllocationType type;
			std::vector<BufferChunk> freeSpace;
		};

		static constexpr uint64_t DEFAULT_BUFFER_SIZE = 2u * 1024u * 1024u;
		static constexpr uint64_t DEFAULT_BUFFER_ALIGNMENT = 64u * 1024u;

		std::vector<BufferSpace> buffers;
		std::vector<BufferSpace> uploadBuffers;
	};
}
