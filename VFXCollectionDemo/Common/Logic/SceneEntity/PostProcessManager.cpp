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
	hdrConstants.middleGray = MIDDLE_GRAY;
	hdrConstants.whiteCutoff = WHITE_CUTOFF;
	hdrConstants.brightThreshold = BRIGHT_THRESHOLD;

	motionBlurConstants.widthU = width;
	motionBlurConstants.area = hdrConstants.area;
	motionBlurConstants.width = static_cast<float>(width);
	motionBlurConstants.height = static_cast<float>(height);

	toneMappingConstants.width = width;
	toneMappingConstants.area = hdrConstants.area;
	toneMappingConstants.halfWidth = width / 2u;
	toneMappingConstants.quartArea = hdrConstants.area / 4u;
	toneMappingConstants.middleGray = MIDDLE_GRAY;
	toneMappingConstants.whiteCutoff = WHITE_CUTOFF;
	toneMappingConstants.brightThreshold = BRIGHT_THRESHOLD;
	toneMappingConstants.bloomIntensity = BLOOM_INTENSITY;

	_width = width;
	_height = height;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	CreateQuad(device, commandList, resourceManager);
	CreateTargets(device, commandList, resourceManager, width, height);
	CreateBuffers(device, commandList, resourceManager, width, height);

	LoadShaders(device, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateComputeObjects(device, resourceManager);

	barriers.reserve(MAX_SIMULTANEOUS_BARRIER_NUMBER);
}

Common::Logic::SceneEntity::PostProcessManager::~PostProcessManager()
{

}

void Common::Logic::SceneEntity::PostProcessManager::Release(ResourceManager* resourceManager)
{
	delete toneMappingMaterial;

	delete motionBlurComputeObject;
	delete luminanceComputeObject;
	delete luminanceIterationComputeObject;
	delete bloomHorizontalObject;
	delete bloomVerticalObject;

	quadMesh->Release(resourceManager);
	delete quadMesh;

	resourceManager->DeleteResource<RenderTarget>(sceneColorTargetId);
	resourceManager->DeleteResource<DepthStencilTarget>(sceneDepthTargetId);

	resourceManager->DeleteResource<RenderTarget>(sceneMotionTargetId);

	resourceManager->DeleteResource<RWBuffer>(sceneBufferId);
	resourceManager->DeleteResource<RWBuffer>(luminanceBufferId);
	resourceManager->DeleteResource<RWBuffer>(bloomBufferId);

	resourceManager->DeleteResource<Shader>(quadVSId);
	resourceManager->DeleteResource<Shader>(motionBlurCSId);
	resourceManager->DeleteResource<Shader>(luminanceCSId);
	resourceManager->DeleteResource<Shader>(luminanceIterationCSId);
	resourceManager->DeleteResource<Shader>(bloomHorizontalCSId);
	resourceManager->DeleteResource<Shader>(bloomVerticalCSId);
	resourceManager->DeleteResource<Shader>(toneMappingPSId);
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
	resourceManager->DeleteResource<RenderTarget>(sceneMotionTargetId);
	resourceManager->DeleteResource<RWBuffer>(sceneBufferId);
	resourceManager->DeleteResource<RWBuffer>(luminanceBufferId);
	resourceManager->DeleteResource<RWBuffer>(bloomBufferId);

	auto commandList = renderer->StartCreatingResources();
	CreateTargets(renderer->GetDevice(), commandList, resourceManager, width, height);
	CreateBuffers(renderer->GetDevice(), commandList, resourceManager, width, height);
	renderer->EndCreatingResources();

	auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	motionBlurComputeObject->UpdateRWBuffer(0u, sceneBufferResource->resourceGPUAddress);

	luminanceComputeObject->UpdateBuffer(0u, sceneBufferResource->resourceGPUAddress);
	luminanceComputeObject->UpdateRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);

	luminanceIterationComputeObject->UpdateRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);

	bloomHorizontalObject->UpdateBuffer(0u, sceneBufferResource->resourceGPUAddress);
	bloomHorizontalObject->UpdateBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	bloomHorizontalObject->UpdateRWBuffer(0u, bloomBufferResource->resourceGPUAddress);

	bloomVerticalObject->UpdateRWBuffer(0u, bloomBufferResource->resourceGPUAddress);

	toneMappingMaterial->UpdateBuffer(0u, sceneBufferResource->resourceGPUAddress);
	toneMappingMaterial->UpdateBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	toneMappingMaterial->UpdateBuffer(2u, bloomBufferResource->resourceGPUAddress);
}

void Common::Logic::SceneEntity::PostProcessManager::SetDepthPrepass(ID3D12GraphicsCommandList* commandList)
{
	commandList->RSSetViewports(1u, &viewport);
	commandList->RSSetScissorRects(1u, &scissorRectangle);

	commandList->ClearDepthStencilView(sceneDepthTargetDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);
	commandList->OMSetRenderTargets(0u, nullptr, true, &sceneDepthTargetDescriptor);
}

void Common::Logic::SceneEntity::PostProcessManager::SetGBuffer(ID3D12GraphicsCommandList* commandList)
{
	sceneColorTargetGPUResource->EndBarrier(commandList);
	
	commandList->ClearRenderTargetView(sceneColorTargetDescriptor, CLEAR_COLOR, 0u, nullptr);
	commandList->OMSetRenderTargets(1u, &sceneColorTargetDescriptor, true, &sceneDepthTargetDescriptor);
}

void Common::Logic::SceneEntity::PostProcessManager::SetDistortBuffer(ID3D12GraphicsCommandList* commandList)
{
	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	if (sceneColorTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (sceneMotionTargetGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	commandList->ClearRenderTargetView(sceneMotionTargetDescriptor, CLEAR_COLOR, 0u, nullptr);
	commandList->OMSetRenderTargets(1u, &sceneMotionTargetDescriptor, true, nullptr);
}

void Common::Logic::SceneEntity::PostProcessManager::Render(ID3D12GraphicsCommandList* commandList)
{
	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	if (sceneColorTargetGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);
	if (sceneMotionTargetGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (sceneBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	motionBlurConstants.widthU = _width;
	motionBlurConstants.area = _width * _height;
	motionBlurConstants.width = static_cast<float>(_width);
	motionBlurConstants.height = static_cast<float>(_height);

	uint32_t numGroupsX = std::max(motionBlurConstants.area / THREADS_PER_GROUP, 1u);

	motionBlurComputeObject->Set(commandList);
	motionBlurComputeObject->SetRootConstants(commandList, 0u, 4u, &motionBlurConstants);
	motionBlurComputeObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	barriers.clear();

	sceneBufferGPUResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);

	if (sceneBufferGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (luminanceBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	hdrConstants.width = _width;
	hdrConstants.area = motionBlurConstants.area;
	uint32_t remain = hdrConstants.area % THREADS_PER_GROUP;
	
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

	barriers.clear();

	luminanceBufferGPUResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);

	if (luminanceBufferGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (bloomBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	toneMappingConstants.width = _width;
	toneMappingConstants.area = _width * _height;
	toneMappingConstants.halfWidth = _width / 2u;
	toneMappingConstants.quartArea = _width * _height / 4u;

	numGroupsX = std::max(toneMappingConstants.quartArea / (THREADS_PER_GROUP - HALF_BLUR_SAMPLES_NUMBER * 2u), 1u);

	bloomHorizontalObject->Set(commandList);
	bloomHorizontalObject->SetRootConstants(commandList, 0u, 7u, &toneMappingConstants);
	bloomHorizontalObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	barriers.clear();

	if (sceneBufferGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (luminanceBufferGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);

	bloomBufferGPUResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	uint32_t verticalBlurConstants[3]
	{
		toneMappingConstants.halfWidth,
		_height / 2u,
		toneMappingConstants.quartArea
	};

	bloomVerticalObject->Set(commandList);
	bloomVerticalObject->SetRootConstants(commandList, 0u, 3u, &verticalBlurConstants);
	bloomVerticalObject->Dispatch(commandList, numGroupsX, 1u, 1u);

	quadMesh->SetInputAssemblerOnly(commandList);

	bloomBufferGPUResource->BeginBarrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Common::Logic::SceneEntity::PostProcessManager::RenderToBackBuffer(ID3D12GraphicsCommandList* commandList)
{
	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	luminanceBufferGPUResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);
	bloomBufferGPUResource->GetUAVBarrier(barrier);
	barriers.push_back(barrier);

	if (sceneBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);
	if (luminanceBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);
	if (bloomBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	toneMappingMaterial->Set(commandList);
	toneMappingMaterial->SetRootConstants(commandList, 0u, 8u, &toneMappingConstants);
	quadMesh->DrawOnly(commandList);

	barriers.clear();

	if (sceneColorTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
		barriers.push_back(barrier);
	if (sceneMotionTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
		barriers.push_back(barrier);
	if (sceneBufferGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
		barriers.push_back(barrier);
	if (luminanceBufferGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
		barriers.push_back(barrier);
	if (bloomBufferGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
}

const Graphics::Assets::Mesh* Common::Logic::SceneEntity::PostProcessManager::GetFullscreenQuadMesh() const
{
	return quadMesh;
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

	sceneTargetDesc.format = DXGI_FORMAT_R16G16_FLOAT;

	sceneMotionTargetId = resourceManager->CreateTextureResource(device, commandList,
		TextureResourceType::RENDER_TARGET, sceneTargetDesc);

	auto sceneColorTargetResource = resourceManager->GetResource<RenderTarget>(sceneColorTargetId);
	auto sceneDepthTargetResource = resourceManager->GetResource<DepthStencilTarget>(sceneDepthTargetId);

	auto sceneDistortionTargetResource = resourceManager->GetResource<RenderTarget>(sceneMotionTargetId);

	sceneColorTargetGPUResource = sceneColorTargetResource->resource;
	sceneMotionTargetGPUResource = sceneDistortionTargetResource->resource;

	sceneColorTargetDescriptor = sceneColorTargetResource->rtvDescriptor.cpuDescriptor;
	sceneDepthTargetDescriptor = sceneDepthTargetResource->dsvDescriptor.cpuDescriptor;

	sceneMotionTargetDescriptor = sceneDistortionTargetResource->rtvDescriptor.cpuDescriptor;
}

void Common::Logic::SceneEntity::PostProcessManager::CreateBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	BufferDesc sceneBufferDesc{};
	sceneBufferDesc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	sceneBufferDesc.dataStride = sizeof(float) * 4u;
	sceneBufferDesc.numElements = width * height;
	sceneBufferDesc.data.resize(static_cast<uint64_t>(sceneBufferDesc.dataStride) * sceneBufferDesc.numElements, 0u);

	sceneBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::RW_BUFFER, sceneBufferDesc);

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

	auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	sceneBufferGPUResource = sceneBufferResource->resource;
	luminanceBufferGPUResource = luminanceBufferResource->resource;
	bloomBufferGPUResource = bloomBufferResource->resource;
}

void Common::Logic::SceneEntity::PostProcessManager::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	quadVSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\QuadVS.hlsl", ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	motionBlurCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\MotionBlurCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	luminanceCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\LuminanceCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	luminanceIterationCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\LuminanceIterationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	bloomHorizontalCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\BloomHorizontalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	bloomVerticalCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\BloomVerticalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	toneMappingPSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\ToneMappingPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);
	
	auto quadVS = resourceManager->GetResource<Shader>(quadVSId);
	auto toneMappingPS = resourceManager->GetResource<Shader>(toneMappingPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetRootConstants(0u, 8u);
	materialBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBuffer(1u, luminanceBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBuffer(2u, bloomBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
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
	auto sceneMotionTargetResource = resourceManager->GetResource<RenderTarget>(sceneMotionTargetId);
	auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);

	auto samplerLinearResource = resourceManager->GetDefaultSampler(device, Graphics::DefaultFilterSetup::FILTER_BILINEAR_CLAMP);

	auto distortionCS = resourceManager->GetResource<Shader>(motionBlurCSId);
	auto luminanceCS = resourceManager->GetResource<Shader>(luminanceCSId);
	auto luminanceIterationCS = resourceManager->GetResource<Shader>(luminanceIterationCSId);
	auto bloomHorizontalCS = resourceManager->GetResource<Shader>(bloomHorizontalCSId);
	auto bloomVerticalCS = resourceManager->GetResource<Shader>(bloomVerticalCSId);

	ComputeObjectBuilder computeObjectBuilder{};
	computeObjectBuilder.SetRootConstants(0u, 4u);
	computeObjectBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);
	computeObjectBuilder.SetTexture(1u, sceneMotionTargetResource->srvDescriptor.gpuDescriptor);
	computeObjectBuilder.SetRWBuffer(0u, sceneBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
	computeObjectBuilder.SetShader(distortionCS->bytecode);

	motionBlurComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 2u);
	computeObjectBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceCS->bytecode);

	luminanceComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 1u);
	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceIterationCS->bytecode);

	luminanceIterationComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 7u);
	computeObjectBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetBuffer(1u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetRWBuffer(0u, bloomBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(bloomHorizontalCS->bytecode);

	bloomHorizontalObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 3u);
	computeObjectBuilder.SetRWBuffer(0u, bloomBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(bloomVerticalCS->bytecode);

	bloomVerticalObject = computeObjectBuilder.Compose(device);
}
