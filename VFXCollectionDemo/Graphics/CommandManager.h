#pragma once

#include "DirectX12Includes.h"

namespace Graphics
{
	using CommandListID = uint32_t;
	using CommandAllocatorID = uint32_t;
	using CommandQueueID = uint32_t;

	class CommandManager
	{
	public:
		CommandManager(ID3D12Device2* _device);
		~CommandManager();

		CommandListID CreateCommandList(CommandAllocatorID allocatorId);
		CommandAllocatorID CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
		CommandQueueID CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);

		ID3D12GraphicsCommandList* BeginRecord(CommandListID commandListId, CommandAllocatorID allocatorId);
		void EndRecord(CommandListID commandList);

		ID3D12CommandQueue* GetQueue(CommandQueueID queueId);

		void SubmitCommandList(CommandListID commandList);
		void ExecuteCommands(CommandQueueID queueId);

		static const size_t MAX_COMMAND_LISTS_TO_SUBMIT = 16;

	private:
		CommandManager() = delete;
		CommandManager(const CommandManager&) = delete;
		CommandManager(CommandManager&&) = delete;
		CommandManager& operator=(const CommandManager&) = delete;
		CommandManager& operator=(CommandManager&&) = delete;

		std::vector<ID3D12GraphicsCommandList*> commandLists;
		std::vector<ID3D12CommandAllocator*> commandAllocators;
		std::vector<ID3D12CommandQueue*> commandQueues;

		std::vector<D3D12_COMMAND_LIST_TYPE> commandListsTypes;
		std::vector<D3D12_COMMAND_LIST_TYPE> commandAllocatorsTypes;
		std::vector<D3D12_COMMAND_LIST_TYPE> commandQueueTypes;

		std::array<ID3D12CommandList*, MAX_COMMAND_LISTS_TO_SUBMIT> commandListsToSubmit;
		uint32_t preparedCommandListsNumber;

		ID3D12Device2* device;
	};
}
