#include "PostProcessManager.h"
#include "MeshObject.h"

#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/ComputeObjectBuilder.h"

using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;
using namespace DirectX::PackedVector;

Common::Logic::SceneEntity::PostProcessManager::PostProcessManager(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRectangle.left = 0;
	scissorRectangle.top = 0;
	scissorRectangle.right = width;
	scissorRectangle.bottom = height;

	hdrConstants.width = width;
	hdrConstants.area = width * height;
	hdrConstants.middleGray = 0.64f;
	hdrConstants.whiteCutoff = 0.8f;
	hdrConstants.brightThreshold = 0.5f;

	_width = width;
	_height = height;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	CreateQuad(device, commandList, resourceManager);
	CreateTargets(device, commandList, resourceManager, width, height);
	CreateBuffers(device, commandList, resourceManager, width, height);
	CreateSamplers(device, resourceManager);

	LoadShaders(device, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateComputeObjects(device, resourceManager);
}

Common::Logic::SceneEntity::PostProcessManager::~PostProcessManager()
{

}

void Common::Logic::SceneEntity::PostProcessManager::Release(ResourceManager* resourceManager)
{
	delete toneMappingMaterial;

	delete luminanceComputeObject;
	delete luminanceIterationComputeObject;
	delete bloomHorizontalObject;
	delete bloomVerticalObject;

	quadMesh->Release(resourceManager);
	delete quadMesh;

	resourceManager->DeleteResource<RenderTarget>(sceneColorTargetId);
	resourceManager->DeleteResource<DepthStencilTarget>(sceneDepthTargetId);

	resourceManager->DeleteResource<RWBuffer>(luminanceBufferId);
	resourceManager->DeleteResource<RWBuffer>(bloomBufferId);

	resourceManager->DeleteResource<Shader>(quadVSId);
	resourceManager->DeleteResource<Shader>(luminanceCSId);
	resourceManager->DeleteResource<Shader>(luminanceIterationCSId);
	resourceManager->DeleteResource<Shader>(bloomHorizontalCSId);
	resourceManager->DeleteResource<Shader>(bloomVerticalCSId);
	resourceManager->DeleteResource<Shader>(toneMappingPSId);

	resourceManager->DeleteResource<Sampler>(samplerPointId);
}

void Common::Logic::SceneEntity::PostProcessManager::OnResize(Graphics::DirectX12Renderer* renderer)
{
	auto width = renderer->GetWidth();
	auto height = renderer->GetHeight();

	_width = width;
	_height = height;

	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);

	scissorRectangle.right = width;
	scissorRectangle.bottom = height;

	auto resourceManager = renderer->GetResourceManager();
	resourceManager->DeleteResource<RenderTarget>(sceneColorTargetId);
	resourceManager->DeleteResource<DepthStencilTarget>(sceneDepthTargetId);
	resourceManager->DeleteResource<RWBuffer>(luminanceBufferId);
	resourceManager->DeleteResource<RWBuffer>(bloomBufferId);

	auto commandList = renderer->StartCreatingResources();
	CreateTargets(renderer->GetDevice(), commandList, resourceManager, width, height);
	CreateBuffers(renderer->GetDevice(), commandList, resourceManager, width, height);
	renderer->EndCreatingResources();

	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	luminanceComputeObject->UpdateRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	luminanceIterationComputeObject->UpdateRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);

	bloomHorizontalObject->UpdateBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	bloomHorizontalObject->UpdateRWBuffer(0u, bloomBufferResource->resourceGPUAddress);

	bloomVerticalObject->UpdateRWBuffer(0u, bloomBufferResource->resourceGPUAddress);

	toneMappingMaterial->UpdateBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	toneMappingMaterial->UpdateBuffer(2u, bloomBufferResource->resourceGPUAddress);
}

void Common::Logic::SceneEntity::PostProcessManager::SetGBuffer(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1u, &viewport);
	commandList->RSSetScissorRects(1u, &scissorRectangle);

	sceneColorTargetGPUResource->EndBarrier(commandList);
	
	commandList->ClearDepthStencilView(sceneDepthTargetDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);
	commandList->ClearRenderTargetView(sceneColorTargetDescriptor, CLEAR_COLOR, 0u, nullptr);
	commandList->OMSetRenderTargets(1u, &sceneColorTargetDescriptor, true, &sceneDepthTargetDescriptor);
}

void Common::Logic::SceneEntity::PostProcessManager::Render(ID3D12GraphicsCommandList* commandList)
{
	sceneColorTargetGPUResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	luminanceBufferGPUResource->EndBarrier(commandList);

	hdrConstants.width = _width;
	hdrConstants.area = _width * _height;
	uint32_t remain = hdrConstants.area % THREADS_PER_GROUP;
	uint32_t numGroupsX = std::max(hdrConstants.area / THREADS_PER_GROUP, 1u);

	luminanceComputeObject->Set(commandList);
	luminanceComputeObject->SetRootConstants(commandList, 0u, 2u, &hdrConstants);
	luminanceComputeObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	if (numGroupsX > 1u)
		luminanceIterationComputeObject->Set(commandList);

	while (numGroupsX > 1u)
	{
		hdrConstants.area = std::max(numGroupsX + remain, 1u);
		remain = numGroupsX % THREADS_PER_GROUP;
		numGroupsX = std::max(numGroupsX / THREADS_PER_GROUP, 1u);
		
		luminanceBufferGPUResource->UAVBarrier(commandList);

		luminanceIterationComputeObject->SetRootConstant(commandList, 0u, &hdrConstants.area);
		luminanceIterationComputeObject->Dispatch(commandList, numGroupsX, 1u, 1u);
	}

	luminanceBufferGPUResource->UAVBarrier(commandList);

	luminanceBufferGPUResource->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	bloomBufferGPUResource->EndBarrier(commandList);

	numGroupsX = std::max(_width * _height / (THREADS_PER_GROUP - HALF_BLUR_SAMPLES_NUMBER * 2u), 1u);

	hdrConstants.width = _width / 2u;
	hdrConstants.area = _width * _height / 4u;

	bloomHorizontalObject->Set(commandList);
	bloomHorizontalObject->SetRootConstants(commandList, 0u, 5u, &hdrConstants);
	bloomHorizontalObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	sceneColorTargetGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	luminanceBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	bloomBufferGPUResource->UAVBarrier(commandList);

	uint32_t verticalBlurConstants[3]
	{
		_width / 2u,
		_height / 2u,
		hdrConstants.area
	};

	bloomVerticalObject->Set(commandList);
	bloomVerticalObject->SetRootConstants(commandList, 0u, 3u, &verticalBlurConstants);
	bloomVerticalObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	quadMesh->SetInputAssemblerOnly(commandList);

	bloomBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Common::Logic::SceneEntity::PostProcessManager::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	luminanceBufferGPUResource->UAVBarrier(commandList);
	bloomBufferGPUResource->UAVBarrier(commandList);

	sceneColorTargetGPUResource->EndBarrier(commandList);
	luminanceBufferGPUResource->EndBarrier(commandList);
	bloomBufferGPUResource->EndBarrier(commandList);

	toneMappingMaterial->Set(commandList);
	toneMappingMaterial->SetRootConstants(commandList, 0u, 4u, &hdrConstants);
	quadMesh->DrawOnly(commandList);

	sceneColorTargetGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	luminanceBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	bloomBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateQuad(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager)
{
	auto halfZero = XMConvertFloatToHalf(0.0f);
	auto halfTwo = XMConvertFloatToHalf(2.0f);

	QuadVertex quadVertices[3] =
	{
		{ float3(-1.0f, 1.0f, 0.5f), XMHALF2(halfZero, halfZero) },
		{ float3(3.0f, 1.0f, 0.5f), XMHALF2(halfTwo, halfZero) },
		{ float3(-1.0f, -3.0f, 0.5f), XMHALF2(halfZero, halfTwo) }
	};

	BufferDesc quadVertexBufferDesc(&quadVertices[0], sizeof(quadVertices));
	auto quadVertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::VERTEX_BUFFER,
		quadVertexBufferDesc);

	uint16_t quadIndices[3] =
	{
		0, 1, 2
	};

	BufferDesc quadIndexBufferDesc(&quadIndices[0], sizeof(quadIndices));
	auto quadIndexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::INDEX_BUFFER,
		quadIndexBufferDesc);

	MeshDesc quadMeshDesc{};
	quadMeshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadMeshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	quadMeshDesc.vertexFormat = Graphics::VertexFormat::POSITION | Graphics::VertexFormat::TEXCOORD0;
	quadMeshDesc.verticesNumber = 3u;
	quadMeshDesc.indicesNumber = 3u;

	quadMesh = new Mesh(quadMeshDesc, quadVertexBufferId, quadIndexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateTargets(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	TextureDesc sceneTargetDesc{};
	sceneTargetDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	sceneTargetDesc.width = width;
	sceneTargetDesc.height = height;
	sceneTargetDesc.depth = 1u;
	sceneTargetDesc.depthBit = 32u;
	sceneTargetDesc.mipLevels = 1u;
	sceneTargetDesc.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	sceneTargetDesc.srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	sceneColorTargetId = resourceManager->CreateTextureResource(device, commandList,
		TextureResourceType::RENDER_TARGET, sceneTargetDesc);

	sceneDepthTargetId = resourceManager->CreateTextureResource(device, commandList,
		TextureResourceType::DEPTH_STENCIL_TARGET, sceneTargetDesc);

	auto sceneColorTargetResource = resourceManager->GetResource<RenderTarget>(sceneColorTargetId);
	auto sceneDepthTargetResource = resourceManager->GetResource<DepthStencilTarget>(sceneDepthTargetId);

	sceneColorTargetGPUResource = sceneColorTargetResource->resource;

	sceneColorTargetDescriptor = sceneColorTargetResource->rtvDescriptor.cpuDescriptor;
	sceneDepthTargetDescriptor = sceneDepthTargetResource->dsvDescriptor.cpuDescriptor;
}

void Common::Logic::SceneEntity::PostProcessManager::CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	BufferDesc luminanceBufferDesc{};
	luminanceBufferDesc.format = DXGI_FORMAT_R32_FLOAT;
	luminanceBufferDesc.dataStride = sizeof(float);
	luminanceBufferDesc.numElements = width * height / THREADS_PER_GROUP;
	luminanceBufferDesc.data.resize(static_cast<uint64_t>(luminanceBufferDesc.dataStride) * luminanceBufferDesc.numElements, 0u);
	
	luminanceBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::RW_BUFFER, luminanceBufferDesc);

	BufferDesc bloomBufferDesc{};
	bloomBufferDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	bloomBufferDesc.dataStride = sizeof(float) * 4u;
	bloomBufferDesc.numElements = width * height / 4u;
	bloomBufferDesc.data.resize(static_cast<uint64_t>(bloomBufferDesc.dataStride) * bloomBufferDesc.numElements, 0u);
	
	bloomBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::RW_BUFFER, bloomBufferDesc);

	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	luminanceBufferGPUResource = luminanceBufferResource->resource;
	bloomBufferGPUResource = bloomBufferResource->resource;
}

void Common::Logic::SceneEntity::PostProcessManager::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	quadVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\QuadVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	luminanceCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\LuminanceCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);
	luminanceIterationCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\LuminanceIterationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);
	bloomHorizontalCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\BloomHorizontalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);
	bloomVerticalCSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\BloomVerticalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	toneMappingPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\ToneMappingPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateSamplers(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager)
{
	samplerPointId = resourceManager->CreateSamplerResource(device,
		Graphics::DirectX12Utilities::CreateSamplerDesc(Graphics::DefaultFilterSetup::FILTER_POINT_CLAMP));
}

void Common::Logic::SceneEntity::PostProcessManager::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto sceneColorTargetResource = resourceManager->GetResource<RenderTarget>(sceneColorTargetId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);
	auto samplerPointResource = resourceManager->GetResource<Sampler>(samplerPointId);

	auto quadVS = resourceManager->GetResource<Shader>(quadVSId);
	auto toneMappingPS = resourceManager->GetResource<Shader>(toneMappingPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetRootConstants(0u, 4u);
	materialBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBuffer(1u, luminanceBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBuffer(2u, bloomBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerPointResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(0u, false);
	materialBuilder.SetRenderTargetFormat(0u, renderer->BACK_BUFFER_FORMAT);
	materialBuilder.SetGeometryFormat(quadMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(quadVS->bytecode);
	materialBuilder.SetPixelShader(toneMappingPS->bytecode);

	toneMappingMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateComputeObjects(ID3D12Device* device, ResourceManager* resourceManager)
{
	auto sceneColorTargetResource = resourceManager->GetResource<RenderTarget>(sceneColorTargetId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	auto luminanceCS = resourceManager->GetResource<Shader>(luminanceCSId);
	auto luminanceIterationCS = resourceManager->GetResource<Shader>(luminanceIterationCSId);
	auto bloomHorizontalCS = resourceManager->GetResource<Shader>(bloomHorizontalCSId);
	auto bloomVerticalCS = resourceManager->GetResource<Shader>(bloomVerticalCSId);

	ComputeObjectBuilder computeObjectBuilder{};
	computeObjectBuilder.SetRootConstants(0u, 2u);
	computeObjectBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);
	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceCS->bytecode);

	luminanceComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 1u);
	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceIterationCS->bytecode);

	luminanceIterationComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 5u);
	computeObjectBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);
	computeObjectBuilder.SetBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetRWBuffer(0u, bloomBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(bloomHorizontalCS->bytecode);

	bloomHorizontalObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 3u);
	computeObjectBuilder.SetRWBuffer(0u, bloomBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(bloomVerticalCS->bytecode);

	bloomVerticalObject = computeObjectBuilder.Compose(device);
}
