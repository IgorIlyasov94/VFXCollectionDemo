#include "CommandManager.h"

Graphics::CommandManager::CommandManager(ID3D12Device2* _device)
	: device(_device), preparedCommandListsNumber(0u)
{
	commandListsToSubmit.fill(nullptr);
}

Graphics::CommandManager::~CommandManager()
{
	for (auto& queue : commandQueues)
		queue->Release();

	for (auto& allocator : commandAllocators)
		allocator->Release();

	for (auto& commandList : commandLists)
		commandList->Release();
}

Graphics::CommandListID Graphics::CommandManager::CreateCommandList(CommandAllocatorID allocatorId)
{
	auto& allocator = commandAllocators[allocatorId];
	auto& allocatorType = commandAllocatorsTypes[allocatorId];

	ID3D12GraphicsCommandList* newCommandList = nullptr;

	device->CreateCommandList(0u, allocatorType, allocator, nullptr, IID_PPV_ARGS(&newCommandList));
	newCommandList->Close();
	
	auto id = static_cast<CommandListID>(commandLists.size());
	
	commandLists.push_back(newCommandList);
	commandListsTypes.push_back(allocatorType);

	return id;
}

Graphics::CommandAllocatorID Graphics::CommandManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator* newCommandAllocator = nullptr;

	device->CreateCommandAllocator(type, IID_PPV_ARGS(&newCommandAllocator));

	auto id = static_cast<CommandAllocatorID>(commandAllocators.size());
	
	commandAllocators.push_back(newCommandAllocator);
	commandAllocatorsTypes.push_back(type);

	return id;
}

Graphics::CommandQueueID Graphics::CommandManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandQueue* commandQueue = nullptr;

	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0u;

	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue));

	CommandQueueID queueId = static_cast<CommandQueueID>(commandQueues.size());

	commandQueues.push_back(commandQueue);
	commandQueueTypes.push_back(type);

	return queueId;
}

ID3D12GraphicsCommandList* Graphics::CommandManager::BeginRecord(CommandListID commandListId, CommandAllocatorID allocatorId)
{
	auto& d3dAllocator = commandAllocators[allocatorId];
	auto& d3dCommandList = commandLists[commandListId];

	d3dAllocator->Reset();
	d3dCommandList->Reset(d3dAllocator, nullptr);

	return d3dCommandList;
}

void Graphics::CommandManager::EndRecord(CommandListID commandListId)
{
	auto& d3dCommandList = commandLists[commandListId];
	d3dCommandList->Close();
}

ID3D12CommandQueue* Graphics::CommandManager::GetQueue(CommandQueueID queueId)
{
	return commandQueues[queueId];
}

void Graphics::CommandManager::SubmitCommandList(CommandListID commandListId)
{
	commandListsToSubmit[preparedCommandListsNumber] = commandLists[commandListId];

	preparedCommandListsNumber++;
}

void Graphics::CommandManager::ExecuteCommands(CommandQueueID queueId)
{
	auto& d3dQueue = commandQueues[queueId];
	d3dQueue->ExecuteCommandLists(preparedCommandListsNumber, commandListsToSubmit.data());

	preparedCommandListsNumber = 0u;
}
