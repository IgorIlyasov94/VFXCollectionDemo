#include "VFXLux.h"
#include "../../../Graphics/Assets/MaterialBuilder.h"
#include "../../../Graphics/Assets/Loaders/DDSLoader.h"

using namespace DirectX;
using namespace Graphics;
using namespace Graphics::Assets;
using namespace Graphics::Assets::Loaders;
using namespace Graphics::Resources;
using namespace DirectX::PackedVector;

Common::Logic::SceneEntity::VFXLux::VFXLux(ID3D12GraphicsCommandList* commandList, DirectX12Renderer* renderer,
	ResourceID perlinNoiseId, Camera* camera)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager);
	LoadShaders(device, resourceManager);

	CreateMesh(device, commandList, resourceManager);
	CreateMaterials(device, resourceManager, perlinNoiseId);
}

Common::Logic::SceneEntity::VFXLux::~VFXLux()
{

}

void Common::Logic::SceneEntity::VFXLux::OnCompute(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{

}

void Common::Logic::SceneEntity::VFXLux::Draw(ID3D12GraphicsCommandList* commandList, float time, float deltaTime)
{
	pillarConstants->viewProjection = _camera->GetViewProjection();
	pillarConstants->time = time;

	pillarMaterial->Set(commandList);
	pillarMesh->Draw(commandList);
}

void Common::Logic::SceneEntity::VFXLux::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete pillarMaterial;

	pillarMesh->Release(resourceManager);
	delete pillarMesh;

	resourceManager->DeleteResource<ConstantBuffer>(pillarConstantsId);
	
	resourceManager->DeleteResource<Shader>(vfxLuxPillarVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxPillarPSId);
}

void Common::Logic::SceneEntity::VFXLux::CreateConstantBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList, Graphics::Resources::ResourceManager* resourceManager)
{
	floatN zero{};
	floatN one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	floatN identityQuaternion = XMQuaternionIdentity();

	pillarWorld = XMMatrixTransformation(zero, identityQuaternion, one, zero, identityQuaternion, zero);

	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXPillarConstants));
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	pillarConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto pillarConstantsResource = resourceManager->GetResource<ConstantBuffer>(pillarConstantsId);
	pillarConstants = reinterpret_cast<VFXPillarConstants*>(pillarConstantsResource->resourceCPUAddress);
	pillarConstants->world = pillarWorld;
	pillarConstants->viewProjection = _camera->GetViewProjection();
	pillarConstants->tiling0 = float2(0.02f, 0.7f);
	pillarConstants->tiling1 = float2(0.08f, 0.07f);
	pillarConstants->scrollSpeed0 = float2(0.05f, 0.2f);
	pillarConstants->scrollSpeed1 = float2(-0.12f, -0.09f);
	pillarConstants->displacementStrength = float4(-0.15f, 0.09f, -0.25f, 1.06f);
	pillarConstants->color0 = float4(1.0f, 0.1239f, 0.0f, 1.0f);
	pillarConstants->color1 = float4(4.2f, 0.7405f, 0.7512f, 1.0f);
	pillarConstants->alphaIntensity = 1.4f;
	pillarConstants->colorIntensity = 0.5f;
	pillarConstants->time = 0.0f;
}

void Common::Logic::SceneEntity::VFXLux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxPillarVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxPillarVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxPillarPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxPillarPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VFXLux::CreateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager)
{
	VFXVertex vertices[VERTICES_NUMBER]{};

	for (uint32_t vertexIndex = 0u; vertexIndex < VERTICES_PER_ROW; vertexIndex++)
	{
		auto& vertex = vertices[vertexIndex];
		vertex.position = float3(GEOMETRY_X_OFFSETS[vertexIndex], 0.0f, GEOMETRY_HEIGHT);

		float u = (vertex.position.x + GEOMETRY_WIDTH / 2.0f) / GEOMETRY_WIDTH;

		vertex.texCoord.x = XMConvertFloatToHalf(u);
		vertex.texCoord.y = XMConvertFloatToHalf(0.0f);

		float rFloat = u * 2.0f - 1.0f;
		auto r = FloatToColorChannel((1.0f - rFloat * rFloat) * 255.0f);

		vertex.color = SetColor(r, 0u, 0u, 0u);
	}

	for (uint32_t vertexIndex = VERTICES_PER_ROW; vertexIndex < (VERTICES_NUMBER - VERTICES_PER_ROW); vertexIndex++)
	{
		auto& vertex = vertices[vertexIndex];

		auto xIndex = vertexIndex % VERTICES_PER_ROW;
		auto yIndex = vertexIndex / VERTICES_PER_ROW;

		float w = GEOMETRY_X_OFFSETS[xIndex];
		float u = (w + GEOMETRY_WIDTH / 2.0f) / GEOMETRY_WIDTH;
		float v = static_cast<float>(yIndex) / static_cast<float>(TOTAL_SEGMENT_NUMBER);
		float h = GEOMETRY_HEIGHT * (1.0f - v);

		vertex.position = float3(w, 0.0f, h);
		vertex.texCoord.x = XMConvertFloatToHalf(u);
		vertex.texCoord.y = XMConvertFloatToHalf(v);

		float rFloat = u * 2.0f - 1.0f;
		float gFloat = v * 2.0f - 1.0f;

		auto r = FloatToColorChannel((1.0f - rFloat * rFloat) * 255.0f);
		auto g = FloatToColorChannel((1.0f - gFloat * gFloat) * 255.0f);
		uint8_t a = (xIndex == 0 || xIndex == 3) ? 0u : 255u;

		vertex.color = SetColor(r, g, 0u, a);
	}

	for (uint32_t vertexIndex = VERTICES_NUMBER - VERTICES_PER_ROW; vertexIndex < VERTICES_NUMBER; vertexIndex++)
	{
		auto& vertex = vertices[vertexIndex];

		auto xIndex = vertexIndex % VERTICES_PER_ROW;

		if (xIndex < VERTICES_PER_ROW)
			vertex.position = float3(GEOMETRY_X_OFFSETS[xIndex], 0.0f, 0.0f);

		float u = (vertex.position.x + GEOMETRY_WIDTH / 2.0f) / GEOMETRY_WIDTH;

		vertex.texCoord.x = XMConvertFloatToHalf(u);
		vertex.texCoord.y = XMConvertFloatToHalf(1.0f);

		float rFloat = u * 2.0f - 1.0f;
		auto r = FloatToColorChannel((1.0f - rFloat * rFloat) * 255.0f);

		vertex.color = SetColor(r, 0u, 0u, 0u);
	}

	uint16_t indices[INDICES_NUMBER]{};

	for (uint32_t index = 0; index < INDICES_NUMBER; index++)
	{
		auto& vertexIndex = indices[index];

		auto localIndex = index % INDICES_PER_QUAD;
		auto rowIndex = index / INDICES_PER_QUAD;
		auto segmentIndex = index / INDICES_PER_SEGMENT;

		vertexIndex = INDEX_OFFSETS[localIndex] + rowIndex + segmentIndex;
	}

	BufferDesc vertexBufferDesc(&vertices[0], sizeof(vertices));

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vertexBufferDesc);

	BufferDesc indexBufferDesc(&indices[0], sizeof(indices));

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, indexBufferDesc);

	MeshDesc meshDesc{};
	meshDesc.verticesNumber = VERTICES_NUMBER;
	meshDesc.indicesNumber = INDICES_NUMBER;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::COLOR0 | VertexFormat::TEXCOORD0;
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	
	pillarMesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VFXLux::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	ResourceID perlinNoiseId)
{
	auto pillarConstantsResource = resourceManager->GetResource<ConstantBuffer>(pillarConstantsId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto vfxLuxPillarVS = resourceManager->GetResource<Shader>(vfxLuxPillarVSId);
	auto vfxLuxPillarPS = resourceManager->GetResource<Shader>(vfxLuxPillarPSId);

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, pillarConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_ALL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(Graphics::DefaultBlendSetup::BLEND_ADDITIVE));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(pillarMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vfxLuxPillarVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxPillarPS->bytecode);

	pillarMaterial = materialBuilder.ComposeStandard(device);
}

uint8_t Common::Logic::SceneEntity::VFXLux::FloatToColorChannel(float value)
{
	return static_cast<uint8_t>(std::min(std::lroundf(value), 255l));
}

uint32_t Common::Logic::SceneEntity::VFXLux::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return static_cast<uint32_t>(a) << 24u | static_cast<uint32_t>(b) << 16u |
		static_cast<uint32_t>(g) << 8u | static_cast<uint32_t>(r);
}
