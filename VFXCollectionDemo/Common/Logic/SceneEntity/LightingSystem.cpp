#include "LightingSystem.h"

using namespace Graphics;
using namespace Graphics::Resources;

Common::Logic::SceneEntity::LightingSystem::LightingSystem(DirectX12Renderer* renderer)
{
	BufferDesc lightConstantBufferDesc{};
	lightConstantBufferDesc.data.resize(sizeof(LightConstantBuffer), 0u);
	lightConstantBufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	auto resourceManager = renderer->GetResourceManager();
	lightConstantBufferId = resourceManager->CreateBufferResource(renderer->GetDevice(), nullptr,
		BufferResourceType::CONSTANT_BUFFER, lightConstantBufferDesc);

	auto lightConstantBufferResource = resourceManager->GetResource<ConstantBuffer>(lightConstantBufferId);

	auto lightConstantBufferData = reinterpret_cast<LightConstantBuffer*>(lightConstantBufferResource->resourceCPUAddress);
	lightSourcesNumberAddress = &lightConstantBufferData->lightSourcesNumber;
	lightBufferAddresses = &lightConstantBufferData->lights[0];
}

Common::Logic::SceneEntity::LightingSystem::~LightingSystem()
{

}

Common::Logic::SceneEntity::LightID Common::Logic::SceneEntity::LightingSystem::CreateLight(const LightDesc& desc)
{
	auto id = static_cast<LightID>(lights.size());
	lights.push_back(desc);

	SetBufferElement(id, desc);
	(*lightSourcesNumberAddress)++;

	return id;
}

void Common::Logic::SceneEntity::LightingSystem::SetSourceDesc(LightID id, const LightDesc& desc)
{
	lights[id] = desc;

	SetBufferElement(id, desc);
}

const Common::Logic::SceneEntity::LightDesc& Common::Logic::SceneEntity::LightingSystem::GetSourceDesc(LightID id) const
{
	return lights[id];
}

Graphics::Resources::ResourceID Common::Logic::SceneEntity::LightingSystem::GetLightConstantBufferId() const
{
	return lightConstantBufferId;
}

void Common::Logic::SceneEntity::LightingSystem::SetBufferElement(LightID id, const LightDesc& desc)
{
	auto& bufferElement = lightBufferAddresses[id];
	bufferElement.position = desc.position;
	bufferElement.direction = desc.direction;
	bufferElement.color = float3(desc.color.x * desc.intensity, desc.color.y * desc.intensity, desc.color.z * desc.intensity);
	bufferElement.type = static_cast<uint32_t>(desc.type);

	if (desc.type == LightType::AREA_LIGHT)
		bufferElement.various0 = desc.radius;
	else if (desc.type == LightType::SPOT_LIGHT)
	{
		bufferElement.various0 = desc.cosPhi2;
		bufferElement.various1 = desc.cosTheta2;
	}
}
