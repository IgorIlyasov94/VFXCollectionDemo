#include "PostProcessManager.h"
#include "MeshObject.h"

#include "../../Graphics/Assets/MaterialBuilder.h"
#include "../../Graphics/Assets/ComputeObjectBuilder.h"
#include "../../Graphics/Assets/Loaders/DDSLoader.h"
#include "../../Graphics/Assets/Generators/NoiseGenerator.h"
#include "../../Graphics/Assets/Generators/TurbulenceMapGenerator.h"

using namespace Graphics::Resources;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;
using namespace Graphics::Assets::Generators;
using namespace DirectX::PackedVector;

Common::Logic::SceneEntity::PostProcessManager::PostProcessManager(ID3D12GraphicsCommandList* commandList,
	Graphics::DirectX12Renderer* renderer, Camera* camera, LightingSystem* lightingSystem,
	const std::vector<DxcDefine>& lightDefines, const RenderingScheme& renderingScheme)
	: _camera(camera), _renderingScheme(renderingScheme)
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
	hdrConstants.middleGray = 0.0f;
	hdrConstants.whiteCutoff = _renderingScheme.whiteCutoff;
	hdrConstants.brightThreshold = _renderingScheme.brightThreshold;

	motionBlurConstants.widthU = width;
	motionBlurConstants.area = hdrConstants.area;
	motionBlurConstants.width = static_cast<float>(width);
	motionBlurConstants.height = static_cast<float>(height);

	toneMappingConstants.width = width;
	toneMappingConstants.area = hdrConstants.area;
	toneMappingConstants.halfWidth = width / 2u;
	toneMappingConstants.quartArea = hdrConstants.area / 4u;
	toneMappingConstants.middleGray = 0.0f;
	toneMappingConstants.whiteCutoff = _renderingScheme.whiteCutoff;
	toneMappingConstants.brightThreshold = _renderingScheme.brightThreshold;
	toneMappingConstants.bloomIntensity = _renderingScheme.bloomIntensity;

	_width = width;
	_height = height;

	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	if (_renderingScheme.enableVolumetricFog)
		CreateConstantBuffers(device, resourceManager, width, height);

	CreateQuad(device, commandList, resourceManager);
	CreateTargets(device, commandList, resourceManager, width, height);
	CreateBuffers(device, commandList, resourceManager, width, height);

	LoadShaders(device, resourceManager, lightDefines);

	if (_renderingScheme.enableVolumetricFog)
		CreateTextures(device, commandList, resourceManager);

	CreateMaterials(device, resourceManager, renderer);
	CreateComputeObjects(device, resourceManager, lightingSystem);

	barriers.reserve(MAX_SIMULTANEOUS_BARRIER_NUMBER);
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

	if (_renderingScheme.enableMotionBlur)
	{
		delete motionBlurComputeObject;

		resourceManager->DeleteResource<RenderTarget>(sceneMotionTargetId);
		resourceManager->DeleteResource<Shader>(motionBlurCSId);
	}

	resourceManager->DeleteResource<RWBuffer>(sceneBufferId);
	resourceManager->DeleteResource<RWBuffer>(luminanceBufferId);
	resourceManager->DeleteResource<RWBuffer>(bloomBufferId);

	if (_renderingScheme.enableVolumetricFog)
	{
		delete volumetricFogComputeObject;

		resourceManager->DeleteResource<Texture>(fogMapId);
		resourceManager->DeleteResource<Texture>(turbulenceMapId);
		resourceManager->DeleteResource<ConstantBuffer>(volumetricFogConstantBufferId);
		resourceManager->DeleteResource<Shader>(volumetricFogCSId);
	}

	resourceManager->DeleteResource<Shader>(quadVSId);
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

	if (_renderingScheme.enableVolumetricFog)
	{
		volumetricFogConstants->widthU = width;
		volumetricFogConstants->area = width * height;
		volumetricFogConstants->width = static_cast<float>(width);
		volumetricFogConstants->height = static_cast<float>(height);
	}

	if (_renderingScheme.enableMotionBlur)
		resourceManager->DeleteResource<RenderTarget>(sceneMotionTargetId);

	resourceManager->DeleteResource<RenderTarget>(sceneColorTargetId);
	resourceManager->DeleteResource<DepthStencilTarget>(sceneDepthTargetId);
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

	if (_renderingScheme.enableMotionBlur)
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
	if (!_renderingScheme.enableDepthPrepass)
		return;

	if (_renderingScheme.enableVolumetricFog)
		sceneDepthTargetGPUResource->EndBarrier(commandList);

	commandList->RSSetViewports(1u, &viewport);
	commandList->RSSetScissorRects(1u, &scissorRectangle);

	commandList->ClearDepthStencilView(sceneDepthTargetDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);
	commandList->OMSetRenderTargets(0u, nullptr, true, &sceneDepthTargetDescriptor);
}

void Common::Logic::SceneEntity::PostProcessManager::SetGBuffer(ID3D12GraphicsCommandList* commandList)
{
	if (!_renderingScheme.enableDepthPrepass)
	{
		commandList->RSSetViewports(1u, &viewport);
		commandList->RSSetScissorRects(1u, &scissorRectangle);
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	if (sceneColorTargetGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);

	if (_renderingScheme.enableVolumetricFog && !_renderingScheme.enableDepthPrepass)
		sceneDepthTargetGPUResource->EndBarrier(commandList);

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	commandList->ClearRenderTargetView(sceneColorTargetDescriptor, CLEAR_COLOR, 0u, nullptr);
	commandList->OMSetRenderTargets(1u, &sceneColorTargetDescriptor, true, &sceneDepthTargetDescriptor);
}

void Common::Logic::SceneEntity::PostProcessManager::SetMotionBuffer(ID3D12GraphicsCommandList* commandList)
{
	if (!_renderingScheme.enableMotionBlur)
		return;

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

	if (_renderingScheme.enableMotionBlur &&
		sceneMotionTargetGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);

	if (_renderingScheme.enableMotionBlur)
	{
		if (sceneColorTargetGPUResource->GetEndBarrier(barrier))
			barriers.push_back(barrier);
	}
	else
	{
		if (sceneColorTargetGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
			barriers.push_back(barrier);
	}

	if (sceneBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);
	if (_renderingScheme.enableVolumetricFog && sceneDepthTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	if (_renderingScheme.enableMotionBlur)
		SetMotionBlur(commandList);

	if (_renderingScheme.enableVolumetricFog)
	{
		barriers.clear();

		if (_renderingScheme.enableMotionBlur)
		{
			sceneBufferGPUResource->GetUAVBarrier(barrier);
			barriers.push_back(barrier);
		}

		if (_renderingScheme.enableVolumetricFog && sceneDepthTargetGPUResource->GetEndBarrier(barrier))
			barriers.push_back(barrier);

		if (!barriers.empty())
			commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

		SetVolumetricFog(commandList);
	}

	SetHDR(commandList);
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

	if (_renderingScheme.enableMotionBlur &&
		sceneMotionTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, barrier))
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

void Common::Logic::SceneEntity::PostProcessManager::CreateConstantBuffers(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, uint32_t width, uint32_t height)
{
	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VolumetricFogConstants), 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	volumetricFogConstantBufferId = resourceManager->CreateBufferResource(device, nullptr, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto volumetricConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(volumetricFogConstantBufferId);
	volumetricFogConstants = reinterpret_cast<VolumetricFogConstants*>(volumetricConstantBufferResource->resourceCPUAddress);
	volumetricFogConstants->invViewProjection = _camera->GetInvViewProjection();
	volumetricFogConstants->widthU = width;
	volumetricFogConstants->area = width * height;
	volumetricFogConstants->width = static_cast<float>(width);
	volumetricFogConstants->height = static_cast<float>(height);
	volumetricFogConstants->cameraDirection = _camera->GetDirection();
	volumetricFogConstants->cameraPosition = _camera->GetPosition();
	volumetricFogConstants->distanceFalloffStart = _renderingScheme.fogDistanceFalloffStart;
	volumetricFogConstants->distanceFalloffLength = _renderingScheme.fogDistanceFalloffLength;
	volumetricFogConstants->fogTiling = _renderingScheme.fogTiling;
	volumetricFogConstants->fogOffset = {};
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
	sceneDepthTargetGPUResource = sceneDepthTargetResource->resource;

	sceneColorTargetDescriptor = sceneColorTargetResource->rtvDescriptor.cpuDescriptor;
	sceneDepthTargetDescriptor = sceneDepthTargetResource->dsvDescriptor.cpuDescriptor;

	if (_renderingScheme.enableMotionBlur)
	{
		sceneTargetDesc.format = DXGI_FORMAT_R16G16_FLOAT;

		sceneMotionTargetId = resourceManager->CreateTextureResource(device, commandList,
			TextureResourceType::RENDER_TARGET, sceneTargetDesc);

		auto sceneDistortionTargetResource = resourceManager->GetResource<RenderTarget>(sceneMotionTargetId);
		sceneMotionTargetGPUResource = sceneDistortionTargetResource->resource;
		sceneMotionTargetDescriptor = sceneDistortionTargetResource->rtvDescriptor.cpuDescriptor;
	}
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

void Common::Logic::SceneEntity::PostProcessManager::LoadShaders(ID3D12Device* device,
	Graphics::Resources::ResourceManager* resourceManager, const std::vector<DxcDefine>& lightDefines)
{
	quadVSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\QuadVS.hlsl", ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	std::vector<DxcDefine> defines;

	if (_renderingScheme.enableMotionBlur)
		defines.push_back({ L"MOTION_BLUR", nullptr });

	if (_renderingScheme.enableVolumetricFog)
		defines.push_back({ L"VOLUMETRIC_FOG", nullptr });

	if (_renderingScheme.enableMotionBlur)
	{
		motionBlurCSId = resourceManager->CreateShaderResource(device,
			"Resources\\Shaders\\PostProcess\\MotionBlurCS.hlsl",
			ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);
	}

	luminanceCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\LuminanceCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5, defines);

	luminanceIterationCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\LuminanceIterationCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	bloomHorizontalCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\BloomHorizontalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5, defines);

	bloomVerticalCSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\BloomVerticalCS.hlsl",
		ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5);

	toneMappingPSId = resourceManager->CreateShaderResource(device,
		"Resources\\Shaders\\PostProcess\\ToneMappingPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5, defines);

	if (_renderingScheme.enableVolumetricFog)
	{
		for (auto& define : lightDefines)
			defines.push_back(define);

		volumetricFogCSId = resourceManager->CreateShaderResource(device,
			"Resources\\Shaders\\PostProcess\\VolumetricFogCS.hlsl",
			ShaderType::COMPUTE_SHADER, ShaderVersion::SM_6_5, defines);
	}
}

void Common::Logic::SceneEntity::PostProcessManager::CreateTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};

	std::filesystem::path fileName("Resources\\Textures\\FogMap.dds");
	
	if (std::filesystem::exists(fileName))
		DDSLoader::Load(fileName, textureDesc);
	else
	{
		NoiseGenerator noiseGenerator{};

		std::vector<float4> noiseData;
		noiseGenerator.Generate(NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, float3(4.0f, 4.0f, 4.0f), noiseData);

		DDSSaveDesc ddsSaveDesc{};
		ddsSaveDesc.width = NOISE_SIZE_X;
		ddsSaveDesc.height = NOISE_SIZE_Y;
		ddsSaveDesc.depth = NOISE_SIZE_Z;
		ddsSaveDesc.targetFormat = DDSFormat::R8G8B8A8_UNORM;
		ddsSaveDesc.dimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		
		DDSLoader::Save(fileName, ddsSaveDesc, noiseData);
		DDSLoader::Load(fileName, textureDesc);
	}

	fogMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);

	fileName = "Resources\\Textures\\TurbulenceMap.dds";

	if (std::filesystem::exists(fileName))
		DDSLoader::Load(fileName, textureDesc);
	else
	{
		TurbulenceMapGenerator turbulenceMapGenerator{};

		std::vector<float4> turbulenceData;
		turbulenceMapGenerator.Generate(NOISE_SIZE_X, NOISE_SIZE_Y, NOISE_SIZE_Z, float3(4.0f, 4.0f, 4.0f), turbulenceData);

		DDSSaveDesc ddsSaveDesc{};
		ddsSaveDesc.width = NOISE_SIZE_X;
		ddsSaveDesc.height = NOISE_SIZE_Y;
		ddsSaveDesc.depth = NOISE_SIZE_Z;
		ddsSaveDesc.targetFormat = DDSFormat::R8G8B8A8_UNORM;
		ddsSaveDesc.dimension = D3D12_SRV_DIMENSION_TEXTURE3D;

		DDSLoader::Save(fileName, ddsSaveDesc, turbulenceData);
		DDSLoader::Load(fileName, textureDesc);
	}

	turbulenceMapId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	Graphics::DirectX12Renderer* renderer)
{
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);
	
	auto quadVS = resourceManager->GetResource<Shader>(quadVSId);
	auto toneMappingPS = resourceManager->GetResource<Shader>(toneMappingPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetRootConstants(0u, 8u);

	if (_renderingScheme.enableMotionBlur || _renderingScheme.enableVolumetricFog)
	{
		auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
		materialBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	}
	else
	{
		auto sceneColorResource = resourceManager->GetResource<Texture>(sceneColorTargetId);
		materialBuilder.SetTexture(0u, sceneColorResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	materialBuilder.SetBuffer(1u, luminanceBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetBuffer(2u, bloomBufferResource->resourceGPUAddress, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_OPAQUE));
	materialBuilder.SetDepthStencilFormat(0u, false, true);
	materialBuilder.SetRenderTargetFormat(0u, renderer->BACK_BUFFER_FORMAT);
	materialBuilder.SetGeometryFormat(quadMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(quadVS->bytecode);
	materialBuilder.SetPixelShader(toneMappingPS->bytecode);

	toneMappingMaterial = materialBuilder.ComposeStandard(device);
}

void Common::Logic::SceneEntity::PostProcessManager::CreateComputeObjects(ID3D12Device* device,
	ResourceManager* resourceManager, LightingSystem* lightingSystem)
{
	auto sceneColorTargetResource = resourceManager->GetResource<RenderTarget>(sceneColorTargetId);
	auto sceneMotionTargetResource = resourceManager->GetResource<RenderTarget>(sceneMotionTargetId);
	auto sceneBufferResource = resourceManager->GetResource<RWBuffer>(sceneBufferId);
	auto luminanceBufferResource = resourceManager->GetResource<RWBuffer>(luminanceBufferId);
	auto bloomBufferResource = resourceManager->GetResource<RWBuffer>(bloomBufferId);
	
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device, Graphics::DefaultFilterSetup::FILTER_BILINEAR_CLAMP);
	
	auto luminanceCS = resourceManager->GetResource<Shader>(luminanceCSId);
	auto luminanceIterationCS = resourceManager->GetResource<Shader>(luminanceIterationCSId);
	auto bloomHorizontalCS = resourceManager->GetResource<Shader>(bloomHorizontalCSId);
	auto bloomVerticalCS = resourceManager->GetResource<Shader>(bloomVerticalCSId);

	ComputeObjectBuilder computeObjectBuilder{};

	if (_renderingScheme.enableMotionBlur)
	{
		auto motionBlurCS = resourceManager->GetResource<Shader>(motionBlurCSId);
		
		computeObjectBuilder.SetRootConstants(0u, 4u);
		computeObjectBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);
		computeObjectBuilder.SetTexture(1u, sceneMotionTargetResource->srvDescriptor.gpuDescriptor);
		computeObjectBuilder.SetRWBuffer(0u, sceneBufferResource->resourceGPUAddress);
		computeObjectBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor);
		computeObjectBuilder.SetShader(motionBlurCS->bytecode);

		motionBlurComputeObject = computeObjectBuilder.Compose(device);
	}

	if (_renderingScheme.enableVolumetricFog)
	{
		auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightingSystem->GetLightConstantBufferId());
		auto volumetricFogConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(volumetricFogConstantBufferId);
		auto sceneDepthTargetResource = resourceManager->GetResource<DepthStencilTarget>(sceneDepthTargetId);

		auto fogMapResource = resourceManager->GetResource<Texture>(fogMapId);
		auto turbulenceMapResource = resourceManager->GetResource<Texture>(turbulenceMapId);

		auto samplerLinearWrapResource = resourceManager->GetDefaultSampler(device, Graphics::DefaultFilterSetup::FILTER_BILINEAR_WRAP);

		auto volumetricFogCS = resourceManager->GetResource<Shader>(volumetricFogCSId);

		computeObjectBuilder.SetConstantBuffer(0u, lightConstantBufferResource->resourceGPUAddress);
		computeObjectBuilder.SetConstantBuffer(1u, volumetricFogConstantBufferResource->resourceGPUAddress);
		computeObjectBuilder.SetTexture(0u, fogMapResource->srvDescriptor.gpuDescriptor);
		computeObjectBuilder.SetTexture(1u, turbulenceMapResource->srvDescriptor.gpuDescriptor);
		computeObjectBuilder.SetTexture(2u, sceneDepthTargetResource->srvDescriptor.gpuDescriptor);

		if (_renderingScheme.useParticleLight)
		{
			auto lightParticleBufferResource = resourceManager->GetResource<RWBuffer>(lightingSystem->GetLightParticleBuffer());

			computeObjectBuilder.SetBuffer(3u, lightParticleBufferResource->resourceGPUAddress);

			if (!_renderingScheme.enableMotionBlur)
				computeObjectBuilder.SetTexture(4u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);
		}
		else if (!_renderingScheme.enableMotionBlur)
			computeObjectBuilder.SetTexture(3u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);

		computeObjectBuilder.SetRWBuffer(0u, sceneBufferResource->resourceGPUAddress);
		computeObjectBuilder.SetSampler(0u, samplerLinearWrapResource->samplerDescriptor.gpuDescriptor);
		computeObjectBuilder.SetShader(volumetricFogCS->bytecode);

		volumetricFogComputeObject = computeObjectBuilder.Compose(device);
	}

	computeObjectBuilder.SetRootConstants(0u, 2u);

	if (_renderingScheme.enableMotionBlur || _renderingScheme.enableVolumetricFog)
		computeObjectBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress);
	else
		computeObjectBuilder.SetTexture(0u, sceneColorTargetResource->srvDescriptor.gpuDescriptor);

	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceCS->bytecode);

	luminanceComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 1u);
	computeObjectBuilder.SetRWBuffer(0u, luminanceBufferResource->resourceGPUAddress);
	computeObjectBuilder.SetShader(luminanceIterationCS->bytecode);

	luminanceIterationComputeObject = computeObjectBuilder.Compose(device);

	computeObjectBuilder.SetRootConstants(0u, 7u);

	if (_renderingScheme.enableMotionBlur || _renderingScheme.enableVolumetricFog)
		computeObjectBuilder.SetBuffer(0u, sceneBufferResource->resourceGPUAddress);
	else
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

void Common::Logic::SceneEntity::PostProcessManager::SetMotionBlur(ID3D12GraphicsCommandList* commandList)
{
	motionBlurConstants.widthU = _width;
	motionBlurConstants.area = _width * _height;
	motionBlurConstants.width = static_cast<float>(_width);
	motionBlurConstants.height = static_cast<float>(_height);

	uint32_t numGroupsX = std::max(motionBlurConstants.area / THREADS_PER_GROUP, 1u);

	motionBlurComputeObject->Set(commandList);
	motionBlurComputeObject->SetRootConstants(commandList, 0u, 4u, &motionBlurConstants);
	motionBlurComputeObject->Dispatch(commandList, numGroupsX, 1u, 1u);
}

void Common::Logic::SceneEntity::PostProcessManager::SetVolumetricFog(ID3D12GraphicsCommandList* commandList)
{
	volumetricFogConstants->invViewProjection = _camera->GetInvViewProjection();
	volumetricFogConstants->cameraPosition = _camera->GetPosition();
	volumetricFogConstants->cameraDirection = _camera->GetDirection();

	if ((_renderingScheme.windStrength != nullptr) && (_renderingScheme.windDirection != nullptr))
	{
		float windStrength = (*_renderingScheme.windStrength) * FOG_MOVING_COEFF;

		volumetricFogConstants->fogOffset.x += _renderingScheme.windDirection->x * windStrength;
		volumetricFogConstants->fogOffset.y += _renderingScheme.windDirection->y * windStrength;
		volumetricFogConstants->fogOffset.z += _renderingScheme.windDirection->z * windStrength;
		volumetricFogConstants->fogOffset.x -= std::floor(volumetricFogConstants->fogOffset.x);
		volumetricFogConstants->fogOffset.y -= std::floor(volumetricFogConstants->fogOffset.y);
		volumetricFogConstants->fogOffset.z -= std::floor(volumetricFogConstants->fogOffset.z);
	}

	uint32_t numGroupsX = std::max(_width * _height / THREADS_PER_GROUP, 1u);

	volumetricFogComputeObject->Set(commandList);
	volumetricFogComputeObject->Dispatch(commandList, numGroupsX, 1u, 1u);
}

void Common::Logic::SceneEntity::PostProcessManager::SetHDR(ID3D12GraphicsCommandList* commandList)
{
	D3D12_RESOURCE_BARRIER barrier{};
	barriers.clear();

	if (_renderingScheme.enableMotionBlur || _renderingScheme.enableVolumetricFog)
	{
		sceneBufferGPUResource->GetUAVBarrier(barrier);
		barriers.push_back(barrier);
	}

	if (sceneBufferGPUResource->GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, barrier))
		barriers.push_back(barrier);
	if (luminanceBufferGPUResource->GetEndBarrier(barrier))
		barriers.push_back(barrier);
	if (_renderingScheme.enableVolumetricFog &&
		sceneDepthTargetGPUResource->GetBeginBarrier(D3D12_RESOURCE_STATE_DEPTH_WRITE, barrier))
		barriers.push_back(barrier);

	commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

	hdrConstants.width = _width;
	hdrConstants.area = motionBlurConstants.area;
	uint32_t remain = hdrConstants.area % THREADS_PER_GROUP;
	uint32_t numGroupsX = std::max(_width * _height / THREADS_PER_GROUP, 1u);

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
