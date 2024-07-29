#include "LightingSystem.h"

using namespace Graphics;
using namespace Graphics::Resources;
using namespace DirectX;

Common::Logic::SceneEntity::LightingSystem::LightingSystem(DirectX12Renderer* renderer)
	: _renderer(renderer), isLightConstantBufferBuilded(false), lightConstantBufferId{},
	lightMatricesConstantBufferId{}, lightMatricesNumber{}
{
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(SHADOW_MAP_SIZE);
	viewport.Height = static_cast<float>(SHADOW_MAP_SIZE);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	cubeViewport.TopLeftX = 0.0f;
	cubeViewport.TopLeftY = 0.0f;
	cubeViewport.Width = static_cast<float>(CUBE_SHADOW_MAP_SIZE);
	cubeViewport.Height = static_cast<float>(CUBE_SHADOW_MAP_SIZE);
	cubeViewport.MinDepth = 0.0f;
	cubeViewport.MaxDepth = 1.0f;

	scissorRectangle.left = 0;
	scissorRectangle.top = 0;
	scissorRectangle.right = SHADOW_MAP_SIZE;
	scissorRectangle.bottom = SHADOW_MAP_SIZE;

	cubeScissorRectangle.left = 0;
	cubeScissorRectangle.top = 0;
	cubeScissorRectangle.right = CUBE_SHADOW_MAP_SIZE;
	cubeScissorRectangle.bottom = CUBE_SHADOW_MAP_SIZE;
}

Common::Logic::SceneEntity::LightingSystem::~LightingSystem()
{

}

Common::Logic::SceneEntity::LightID Common::Logic::SceneEntity::LightingSystem::CreateLight(const LightDesc& desc)
{
	auto id = static_cast<LightID>(lights.size());
	lights.push_back(desc);

	auto& light = lights[id];

	if (light.castShadows)
	{
		light.shadowMapId = CreateShadowMap(light.type == LightType::POINT_LIGHT || light.type == LightType::AREA_LIGHT);
		
		auto resourceManager = _renderer->GetResourceManager();
		auto shadowMap = resourceManager->GetResource<DepthStencilTarget>(light.shadowMapId);

		light.shadowMapResource = shadowMap->resource;
		light.shadowMapCPUDescriptor = shadowMap->dsvDescriptor.cpuDescriptor;
		barriers.reserve(barriers.size() + 1u);

		light.lightMatrixStartIndex = lightMatricesNumber;
		lightMatricesNumber += (light.type == LightType::POINT_LIGHT || light.type == LightType::AREA_LIGHT) ? 6u : 1u;
	}

	return id;
}

void Common::Logic::SceneEntity::LightingSystem::SetSourceDesc(LightID id, const LightDesc& desc)
{
	auto& light = lights[id];

	light.position = desc.position;
	light.radius = desc.radius;
	light.color = desc.color;
	light.intensity = desc.intensity;
	light.direction = desc.direction;
	light.cosPhi2 = desc.cosPhi2;
	light.cosTheta2 = desc.cosTheta2;
	light.falloff = desc.falloff;

	if (light.castShadows)
	{
		SetupViewProjectMatrices(light);
		SetLightBufferElement(light);
	}
}

const Common::Logic::SceneEntity::LightDesc& Common::Logic::SceneEntity::LightingSystem::GetSourceDesc(LightID id) const
{
	return lights[id];
}

uint32_t Common::Logic::SceneEntity::LightingSystem::GetLightMatricesNumber() const
{
	return lightMatricesNumber;
}

Graphics::Resources::ResourceID Common::Logic::SceneEntity::LightingSystem::GetLightConstantBufferId()
{
	if (!isLightConstantBufferBuilded)
	{
		CreateConstantBuffers();

		isLightConstantBufferBuilded = true;
	}

	return lightConstantBufferId;
}

Graphics::Resources::ResourceID Common::Logic::SceneEntity::LightingSystem::GetLightMatricesConstantBufferId()
{
	if (!isLightConstantBufferBuilded)
	{
		CreateConstantBuffers();

		isLightConstantBufferBuilded = true;
	}

	return lightMatricesConstantBufferId;
}

void Common::Logic::SceneEntity::LightingSystem::BeforeStartRenderShadowMaps(ID3D12GraphicsCommandList* commandList)
{
	barriers.clear();

	for (auto& lightDesc : lights)
	{
		if (!lightDesc.castShadows)
			continue;

		D3D12_RESOURCE_BARRIER barrier;
		if (lightDesc.shadowMapResource->GetEndBarrier(barrier))
			barriers.push_back(barrier);
	}

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
}

void Common::Logic::SceneEntity::LightingSystem::StartRenderShadowMap(LightID id, ID3D12GraphicsCommandList* commandList)
{
	auto& lightDesc = lights[id];

	if (lightDesc.type == LightType::POINT_LIGHT || lightDesc.type == LightType::AREA_LIGHT)
	{
		commandList->RSSetViewports(1u, &cubeViewport);
		commandList->RSSetScissorRects(1u, &cubeScissorRectangle);
	}
	else
	{
		commandList->RSSetViewports(1u, &viewport);
		commandList->RSSetScissorRects(1u, &scissorRectangle);
	}

	lightDesc.shadowMapResource->EndBarrier(commandList);

	commandList->ClearDepthStencilView(lightDesc.shadowMapCPUDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr);
	commandList->OMSetRenderTargets(0u, nullptr, true, &lightDesc.shadowMapCPUDescriptor);
}

void Common::Logic::SceneEntity::LightingSystem::EndRenderShadowMaps(ID3D12GraphicsCommandList* commandList)
{
	barriers.clear();

	for (auto& lightDesc : lights)
	{
		if (!lightDesc.castShadows)
			continue;

		D3D12_RESOURCE_BARRIER barrier;
		if (lightDesc.shadowMapResource->GetBarrier(D3D12_RESOURCE_STATE_DEPTH_READ |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, barrier))
			barriers.push_back(barrier);
	}

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
}

void Common::Logic::SceneEntity::LightingSystem::EndUsingShadowMaps(ID3D12GraphicsCommandList* commandList)
{
	barriers.clear();

	for (auto& lightDesc : lights)
	{
		if (!lightDesc.castShadows)
			continue;

		D3D12_RESOURCE_BARRIER barrier;
		lightDesc.shadowMapResource->GetBeginBarrier(D3D12_RESOURCE_STATE_DEPTH_WRITE, barrier);
		barriers.push_back(barrier);
	}

	if (!barriers.empty())
		commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
}

void Common::Logic::SceneEntity::LightingSystem::Clear()
{
	auto resourceManager = _renderer->GetResourceManager();

	for (auto& lightDesc : lights)
		if (lightDesc.castShadows)
			resourceManager->DeleteResource<DepthStencilTarget>(lightDesc.shadowMapId);

	lights.clear();

	resourceManager->DeleteResource<ConstantBuffer>(lightConstantBufferId);
	resourceManager->DeleteResource<ConstantBuffer>(lightMatricesConstantBufferId);
}

uint32_t Common::Logic::SceneEntity::LightingSystem::GetSizeOfType(LightType type)
{
	if (type == LightType::DIRECTIONAL_LIGHT)
		return static_cast<uint32_t>(sizeof(DirectionalLight));
	else if (type == LightType::POINT_LIGHT)
		return static_cast<uint32_t>(sizeof(PointLight));
	else if (type == LightType::AREA_LIGHT)
		return static_cast<uint32_t>(sizeof(AreaLight));
	else if (type == LightType::SPOT_LIGHT)
		return static_cast<uint32_t>(sizeof(SpotLight));
	else if (type == LightType::AMBIENT_LIGHT)
		return static_cast<uint32_t>(sizeof(AmbientLight));

	return 0u;
}

uint32_t Common::Logic::SceneEntity::LightingSystem::CalculateLightBufferSize()
{
	uint32_t size = 0u;

	for (auto& lightDesc : lights)
		size += GetSizeOfType(lightDesc.type);

	return size;
}

void Common::Logic::SceneEntity::LightingSystem::SetLightBufferElement(LightDesc& desc)
{
	if (desc.type == LightType::DIRECTIONAL_LIGHT)
	{
		auto light = reinterpret_cast<DirectionalLight*>(desc.lightBufferStartAddress);
		light->direction = desc.direction;
		light->color = desc.color;
	}
	else if (desc.type == LightType::POINT_LIGHT)
	{
		auto light = reinterpret_cast<PointLight*>(desc.lightBufferStartAddress);
		light->position = desc.position;
		light->color = float3(desc.color.x * desc.intensity, desc.color.y * desc.intensity, desc.color.z * desc.intensity);
	}
	else if (desc.type == LightType::AREA_LIGHT)
	{
		auto light = reinterpret_cast<AreaLight*>(desc.lightBufferStartAddress);
		light->position = desc.position;
		light->radius = desc.radius;
		light->color = float3(desc.color.x * desc.intensity, desc.color.y * desc.intensity, desc.color.z * desc.intensity);
	}
	else if (desc.type == LightType::SPOT_LIGHT)
	{
		auto light = reinterpret_cast<SpotLight*>(desc.lightBufferStartAddress);
		light->position = desc.position;
		light->direction = desc.direction;
		light->color = float3(desc.color.x * desc.intensity, desc.color.y * desc.intensity, desc.color.z * desc.intensity);
		light->falloff = desc.falloff;
		light->cosPhi2 = desc.cosPhi2;
		light->cosTheta2 = desc.cosTheta2;
	}
	else if (desc.type == LightType::AMBIENT_LIGHT)
	{
		auto light = reinterpret_cast<AmbientLight*>(desc.lightBufferStartAddress);
		light->color = float3(desc.color.x * desc.intensity, desc.color.y * desc.intensity, desc.color.z * desc.intensity);
	}
}

Graphics::Resources::ResourceID Common::Logic::SceneEntity::LightingSystem::CreateShadowMap(bool isCube)
{
	TextureDesc shadowMapDesc{};
	shadowMapDesc.width = isCube ? CUBE_SHADOW_MAP_SIZE : SHADOW_MAP_SIZE;
	shadowMapDesc.height = isCube ? CUBE_SHADOW_MAP_SIZE : SHADOW_MAP_SIZE;
	shadowMapDesc.depth = isCube ? 6u : 1u;
	shadowMapDesc.mipLevels = 1u;
	shadowMapDesc.depthBit = 32u;
	shadowMapDesc.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	shadowMapDesc.srvDimension = isCube ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
	
	auto resourceManager = _renderer->GetResourceManager();
	auto shadowMapId = resourceManager->CreateTextureResource(_renderer->GetDevice(), nullptr,
		TextureResourceType::DEPTH_STENCIL_TARGET, shadowMapDesc);

	return shadowMapId;
}

void Common::Logic::SceneEntity::LightingSystem::CreateConstantBuffers()
{
	BufferDesc lightBufferDesc{};
	lightBufferDesc.data.resize(CalculateLightBufferSize(), 0u);
	lightBufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	auto resourceManager = _renderer->GetResourceManager();
	lightConstantBufferId = resourceManager->CreateBufferResource(_renderer->GetDevice(), nullptr,
		BufferResourceType::CONSTANT_BUFFER, lightBufferDesc);

	auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightConstantBufferId);
	auto lightConstantBufferPtr = lightConstantBufferResource->resourceCPUAddress;

	for (auto& lightDesc : lights)
	{
		lightDesc.lightBufferStartAddress = lightConstantBufferPtr;
		SetLightBufferElement(lightDesc);

		lightConstantBufferPtr += GetSizeOfType(lightDesc.type);
	}

	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(float4x4) * lightMatricesNumber, 0u);
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	lightMatricesConstantBufferId = resourceManager->CreateBufferResource(_renderer->GetDevice(), nullptr,
		BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto lightMatricesConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightMatricesConstantBufferId);
	auto lightMatricesConstantBufferPtr = reinterpret_cast<float4x4*>(lightMatricesConstantBufferResource->resourceCPUAddress);

	for (auto& lightDesc : lights)
	{
		lightDesc.viewProjections = lightMatricesConstantBufferPtr;
		SetupViewProjectMatrices(lightDesc);

		auto isCube = lightDesc.type == LightType::POINT_LIGHT || lightDesc.type == LightType::AREA_LIGHT;
		lightMatricesConstantBufferPtr += isCube ? 6u : 1u;
	}
}

void Common::Logic::SceneEntity::LightingSystem::SetupViewProjectMatrices(LightDesc& desc)
{
	if (desc.type == LightType::POINT_LIGHT || desc.type == LightType::AREA_LIGHT)
	{
		desc.viewProjections[0u] = BuildViewProjectMatrix(desc.position, float3(1.0f, 0.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f), CUBE_SHADOW_MAP_SIZE, false);
		desc.viewProjections[1u] = BuildViewProjectMatrix(desc.position, float3(-1.0f, 0.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f), CUBE_SHADOW_MAP_SIZE, false);
		desc.viewProjections[2u] = BuildViewProjectMatrix(desc.position, float3(0.0f, 0.0f, 1.0f),
			float3(0.0f, -1.0f, 0.0f), CUBE_SHADOW_MAP_SIZE, false);
		desc.viewProjections[3u] = BuildViewProjectMatrix(desc.position, float3(0.0f, 0.0f, -1.0f),
			float3(0.0f, 1.0f, 0.0f), CUBE_SHADOW_MAP_SIZE, false);
		desc.viewProjections[4u] = BuildViewProjectMatrix(desc.position, float3(0.0f, 1.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f), CUBE_SHADOW_MAP_SIZE, false);
		desc.viewProjections[5u] = BuildViewProjectMatrix(desc.position, float3(0.0f, -1.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f), CUBE_SHADOW_MAP_SIZE, false);
	}
	else if (desc.type != LightType::AMBIENT_LIGHT)
	{
		*desc.viewProjections = BuildViewProjectMatrix(desc.position, desc.direction,
			float3(0.0f, 0.0f, 1.0f), CUBE_SHADOW_MAP_SIZE, desc.type == LightType::DIRECTIONAL_LIGHT);
	}
}

float4x4 Common::Logic::SceneEntity::LightingSystem::BuildViewProjectMatrix(const float3& position,
	const float3& direction, const float3& up, uint32_t size, bool isDirectional)
{
	auto positionN = XMLoadFloat3(&position);
	auto directionN = XMLoadFloat3(&direction);
	auto upN = XMLoadFloat3(&up);

	auto view = XMMatrixLookToRH(positionN, directionN, upN);
	float4x4 projection{};

	if (isDirectional)
		projection = XMMatrixOrthographicRH(static_cast<float>(size), static_cast<float>(size), SHADOW_MAP_Z_NEAR, SHADOW_MAP_Z_FAR);
	else
		projection = XMMatrixPerspectiveFovRH(static_cast<float>(std::numbers::pi * 0.5), 1.0f, SHADOW_MAP_Z_NEAR, SHADOW_MAP_Z_FAR);

	return view * projection;
}
