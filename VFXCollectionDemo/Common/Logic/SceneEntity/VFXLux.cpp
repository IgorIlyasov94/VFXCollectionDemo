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
	ResourceID perlinNoiseId, Camera* camera, const float3& position)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;

	CreateConstantBuffers(device, commandList, resourceManager, position);
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
	circleConstants->viewProjection = _camera->GetViewProjection();
	circleConstants->invView = _camera->GetInvView();
	circleConstants->time = time;

	circleMaterial->Set(commandList);
	circleMesh->Draw(commandList);
}

void Common::Logic::SceneEntity::VFXLux::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete circleMaterial;

	circleMesh->Release(resourceManager);
	delete circleMesh;

	resourceManager->DeleteResource<ConstantBuffer>(circleConstantsId);
	
	resourceManager->DeleteResource<Shader>(vfxLuxCircleVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxCirclePSId);
}

void Common::Logic::SceneEntity::VFXLux::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, const float3& position)
{
	floatN zero{};
	floatN one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	floatN identityQuaternion = XMQuaternionIdentity();

	circleWorld = XMMatrixTransformation(zero, identityQuaternion, one, zero, identityQuaternion, zero);

	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXPillarConstants));
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	circleConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto circleConstantsResource = resourceManager->GetResource<ConstantBuffer>(circleConstantsId);
	circleConstants = reinterpret_cast<VFXPillarConstants*>(circleConstantsResource->resourceCPUAddress);
	circleConstants->invView = _camera->GetInvView();
	circleConstants->viewProjection = _camera->GetViewProjection();
	circleConstants->color0 = float4(1.0f, 0.1239f, 0.0f, 1.0f);
	circleConstants->color1 = float4(1.0f, 0.7405f, 0.9812f, 1.0f);
	circleConstants->worldPosition = position;
	circleConstants->time = 0.0f;
	circleConstants->tiling0 = float2(1.0f, 0.5f);
	circleConstants->tiling1 = float2(1.0f, 0.35f);
	circleConstants->scrollSpeed0 = float2(0.094f, 0.05f);
	circleConstants->scrollSpeed1 = float2(-0.107f, -0.046f);
	circleConstants->colorIntensity = 1.6f;
	circleConstants->alphaSharpness = 3.0f;
	circleConstants->distortionStrength = 0.15f;
	circleConstants->padding = 0.0f;
}

void Common::Logic::SceneEntity::VFXLux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxCircleVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxCircleVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxCirclePSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxCirclePS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VFXLux::CreateMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager)
{
	VFXVertex vertices[VERTICES_NUMBER]{};

	for (uint32_t ringIndex = 0u; ringIndex < TOTAL_RING_NUMBER; ringIndex++)
		for (uint32_t ringVertexIndex = 0u; ringVertexIndex < VERTICES_PER_RING; ringVertexIndex++)
		{
			auto vertexIndex = ringIndex * VERTICES_PER_RING + ringVertexIndex;
			auto& vertex = vertices[vertexIndex];
			auto angle = ringVertexIndex * 2.0f * static_cast<float>(std::numbers::pi) / (VERTICES_PER_RING - 1);
			auto radius = RING_RADIUSES[ringIndex];
			
			vertex.position = float3(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);

			auto u = ringVertexIndex / static_cast<float>(VERTICES_PER_RING - 1);
			auto v = (radius - RING_START_OFFSET) / CIRCLE_WIDTH;

			vertex.texCoord.x = XMConvertFloatToHalf(u);
			vertex.texCoord.y = XMConvertFloatToHalf(v);

			auto r = FloatToColorChannel((TOTAL_RING_NUMBER - ringIndex - 1) * 255.0f / (TOTAL_RING_NUMBER - 1));
			auto a = FloatToColorChannel((ringIndex == 1u || ringIndex == 2u) ? 255.0f : 0.0f);

			vertex.color = SetColor(r, 0u, 0u, a);
		}

	uint16_t indices[INDICES_NUMBER]{};
	uint32_t indexIterator = 0u;

	for (uint32_t ribbonIndex = 0; ribbonIndex < TOTAL_RIBBON_NUMBER; ribbonIndex++)
	{
		auto ringOffset = ribbonIndex * VERTICES_PER_RING;

		for (uint32_t segmentIndex = 0; segmentIndex < TOTAL_SEGMENT_NUMBER; segmentIndex++)
		{
			auto offset = segmentIndex + ringOffset;

			for (uint32_t index = 0; index < INDICES_PER_SEGMENT; index++)
				indices[indexIterator++] = INDEX_OFFSETS[index] + offset;
		}
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
	
	circleMesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VFXLux::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	ResourceID perlinNoiseId)
{
	auto pillarConstantsResource = resourceManager->GetResource<ConstantBuffer>(circleConstantsId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto vfxLuxCircleVS = resourceManager->GetResource<Shader>(vfxLuxCircleVSId);
	auto vfxLuxCirclePS = resourceManager->GetResource<Shader>(vfxLuxCirclePSId);

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, pillarConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(circleMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vfxLuxCircleVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxCirclePS->bytecode);

	circleMaterial = materialBuilder.ComposeStandard(device);
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
