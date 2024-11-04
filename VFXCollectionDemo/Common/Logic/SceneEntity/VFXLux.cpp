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
	ResourceID perlinNoiseId, ResourceID volumeNoiseId, ResourceID turbulenceMapId, ResourceID vfxAtlasId,
	Camera* camera, const float3& position)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	_camera = camera;
	
	CreateConstantBuffers(device, commandList, resourceManager, position);
	LoadShaders(device, resourceManager);
	LoadTextures(device, commandList, resourceManager);

	CreateMeshes(device, commandList, resourceManager);
	CreateMaterials(device, resourceManager, perlinNoiseId, volumeNoiseId, turbulenceMapId, vfxAtlasId);
}

Common::Logic::SceneEntity::VFXLux::~VFXLux()
{

}

void Common::Logic::SceneEntity::VFXLux::Update(float time, float deltaTime)
{
	circleConstants->viewProjection = _camera->GetViewProjection();
	circleConstants->invView = _camera->GetInvView();
	circleConstants->time = time;
	circleConstants->colorIntensity = CIRCLE_INTENSITY + std::pow(std::max(std::sin(time * 0.4f), 0.0f), 60.0f) * 0.0f;

	haloConstants->viewProjection = _camera->GetViewProjection();
	haloConstants->invView = _camera->GetInvView();
	haloConstants->time = time;
	haloConstants->colorIntensity = HALO_INTENSITY;
}

void Common::Logic::SceneEntity::VFXLux::OnCompute(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::VFXLux::DrawDepthPrepass(ID3D12GraphicsCommandList* commandList)
{

}

void Common::Logic::SceneEntity::VFXLux::DrawShadows(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLux::DrawShadowsCube(ID3D12GraphicsCommandList* commandList,
	uint32_t lightMatrixStartIndex)
{

}

void Common::Logic::SceneEntity::VFXLux::Draw(ID3D12GraphicsCommandList* commandList)
{
	circleMaterial->Set(commandList);
	circleMesh->Draw(commandList);

	haloMaterial->Set(commandList);
	haloMesh->Draw(commandList);
}

void Common::Logic::SceneEntity::VFXLux::Release(Graphics::Resources::ResourceManager* resourceManager)
{
	delete circleMaterial;
	delete haloMaterial;

	circleMesh->Release(resourceManager);
	haloMesh->Release(resourceManager);
	delete circleMesh;
	delete haloMesh;

	resourceManager->DeleteResource<ConstantBuffer>(circleConstantsId);
	resourceManager->DeleteResource<ConstantBuffer>(haloConstantsId);

	resourceManager->DeleteResource<Shader>(vfxLuxCircleVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxHaloVSId);
	resourceManager->DeleteResource<Shader>(vfxLuxCirclePSId);
	resourceManager->DeleteResource<Shader>(vfxLuxHaloPSId);
}

void Common::Logic::SceneEntity::VFXLux::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager, const float3& position)
{
	floatN zero{};
	floatN one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	floatN identityQuaternion = XMQuaternionIdentity();

	circleWorld = XMMatrixTransformation(zero, identityQuaternion, one, zero, identityQuaternion, zero);

	BufferDesc bufferDesc{};
	bufferDesc.data.resize(sizeof(VFXCircleConstants));
	bufferDesc.flag = BufferFlag::IS_CONSTANT_DYNAMIC;

	circleConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto circleConstantsResource = resourceManager->GetResource<ConstantBuffer>(circleConstantsId);
	circleConstants = reinterpret_cast<VFXCircleConstants*>(circleConstantsResource->resourceCPUAddress);
	circleConstants->invView = _camera->GetInvView();
	circleConstants->viewProjection = _camera->GetViewProjection();
	circleConstants->color0 = float3(10.0f, 2.0f, 1.0f);
	circleConstants->color1 = float3(0.6f, 0.8f, 1.0f);
	circleConstants->worldPosition = position;
	circleConstants->time = 0.0f;
	circleConstants->tiling0 = float2(1.0f, 1.0f);
	circleConstants->tiling1 = float2(1.0f, 1.0f);
	circleConstants->scrollSpeed0 = float2(0.015f, -0.029f);
	circleConstants->scrollSpeed1 = float2(-0.002f, -0.046f);
	circleConstants->colorIntensity = 0.0f;
	circleConstants->distortionStrength = CIRCLE_DISTORTION_INTENSITY;

	bufferDesc.data.resize(sizeof(VFXHaloConstants));

	haloConstantsId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::CONSTANT_BUFFER, bufferDesc);

	auto haloConstantsResource = resourceManager->GetResource<ConstantBuffer>(haloConstantsId);
	haloConstants = reinterpret_cast<VFXHaloConstants*>(haloConstantsResource->resourceCPUAddress);
	haloConstants->invView = _camera->GetInvView();
	haloConstants->viewProjection = _camera->GetViewProjection();
	haloConstants->worldPosition = position;
	haloConstants->time = 0.0f;
	haloConstants->tiling0 = float2(0.3f, 0.15f);
	haloConstants->tiling1 = float2(0.2f, 0.1f);
	haloConstants->scrollSpeed0 = float2(0.0f, -0.08f);
	haloConstants->scrollSpeed1 = float2(-0.01f, -0.015f);
	haloConstants->colorIntensity = 0.0f;
	haloConstants->alphaSharpness = HALO_SHARPNESS;
	haloConstants->distortionStrength = 0.1f;
	haloConstants->padding = 0.0f;
}

void Common::Logic::SceneEntity::VFXLux::LoadShaders(ID3D12Device* device, Graphics::Resources::ResourceManager* resourceManager)
{
	vfxLuxCircleVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxCircleVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxHaloVSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxHaloVS.hlsl",
		ShaderType::VERTEX_SHADER, ShaderVersion::SM_6_5);

	vfxLuxHaloPSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxHaloPS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);

	vfxLuxCirclePSId = resourceManager->CreateShaderResource(device, "Resources\\Shaders\\FX\\VFXLuxCirclePS.hlsl",
		ShaderType::PIXEL_SHADER, ShaderVersion::SM_6_5);
}

void Common::Logic::SceneEntity::VFXLux::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	TextureDesc textureDesc{};
	DDSLoader::Load("Resources\\Textures\\LuxHaloSpectrum.dds", textureDesc);
	vfxLuxHaloSpectrumId = resourceManager->CreateTextureResource(device, commandList, TextureResourceType::TEXTURE, textureDesc);
}

void Common::Logic::SceneEntity::VFXLux::CreateMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager)
{
	CreateCircleMesh(device, commandList, resourceManager);
	CreateHaloMesh(device, commandList, resourceManager);
}

void Common::Logic::SceneEntity::VFXLux::CreateCircleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
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
			auto g = FloatToColorChannel(255.0f - v * 255.0f);
			auto a = FloatToColorChannel((ringIndex == 1u || ringIndex == 2u) ? 255.0f : 0.0f);

			vertex.color = SetColor(r, g, 0u, a);
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

void Common::Logic::SceneEntity::VFXLux::CreateHaloMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{
	VFXVertex vertices[HALO_VERTICES_NUMBER]{};

	constexpr auto segmentAngle = 2.0f * static_cast<float>(std::numbers::pi) / (VERTICES_PER_RING - 1);

	for (uint32_t ringIndex = 0u; ringIndex < TOTAL_RING_NUMBER; ringIndex++)
		for (uint32_t ringVertexIndex = 0u; ringVertexIndex < HALO_VERTICES_PER_RING; ringVertexIndex++)
		{
			auto vertexIndex = ringIndex * HALO_VERTICES_PER_RING + ringVertexIndex;
			auto& vertex = vertices[vertexIndex];
			auto angle = ringVertexIndex * segmentAngle;
			angle += (ringVertexIndex >= HALO_VERTICES_PER_SECTOR) ? -segmentAngle : -segmentAngle * 2.0f;

			auto radius = HALO_RING_RADIUSES[ringIndex];

			vertex.position = float3(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);

			auto u = ringVertexIndex / static_cast<float>(HALO_VERTICES_PER_RING);
			auto v = (radius - HALO_RING_START_OFFSET) / HALO_WIDTH;

			vertex.texCoord.x = XMConvertFloatToHalf(u);
			vertex.texCoord.y = XMConvertFloatToHalf(v);

			auto isOpaque = ringIndex == 1u || ringIndex == 2u;
			isOpaque = isOpaque && (ringVertexIndex != 0) && (ringVertexIndex != 4) &&
				(ringVertexIndex != 5) && (ringVertexIndex != 9);

			auto isSemitransparent = ringIndex == 1u || ringIndex == 2u;
			isSemitransparent = isSemitransparent && (ringVertexIndex == 1 || ringVertexIndex == 3 ||
				ringVertexIndex == 6 || ringVertexIndex == 8);

			auto r = FloatToColorChannel((TOTAL_RING_NUMBER - ringIndex - 1) * 255.0f / (TOTAL_RING_NUMBER - 1));
			auto g = static_cast<uint8_t>(255u - r);
			auto a = FloatToColorChannel(isOpaque ? (isSemitransparent ? 196u : 255u) : 0.0f);

			vertex.color = SetColor(r, g, 0u, a);
		}

	uint16_t indices[HALO_INDICES_NUMBER]{};
	uint32_t indexIterator = 0u;

	for (uint32_t ribbonIndex = 0; ribbonIndex < TOTAL_RIBBON_NUMBER; ribbonIndex++)
	{
		auto ringOffset = ribbonIndex * HALO_VERTICES_PER_RING;

		for (uint32_t segmentIndex = 0; segmentIndex < HALO_SEGMENT_NUMBER; segmentIndex++)
		{
			auto offset = segmentIndex + ringOffset;
			offset += segmentIndex >= HALO_SEGMENTS_PER_SECTOR ? 1u : 0u;

			for (uint32_t index = 0; index < INDICES_PER_SEGMENT; index++)
				indices[indexIterator++] = HALO_INDEX_OFFSETS[index] + offset;
		}
	}

	BufferDesc vertexBufferDesc(&vertices[0], sizeof(vertices));

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vertexBufferDesc);

	BufferDesc indexBufferDesc(&indices[0], sizeof(indices));

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, indexBufferDesc);

	MeshDesc meshDesc{};
	meshDesc.verticesNumber = HALO_VERTICES_NUMBER;
	meshDesc.indicesNumber = HALO_INDICES_NUMBER;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::COLOR0 | VertexFormat::TEXCOORD0;
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	haloMesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VFXLux::CreateMaterials(ID3D12Device* device, ResourceManager* resourceManager,
	ResourceID perlinNoiseId, ResourceID volumeNoiseId, ResourceID turbulenceMapId, ResourceID vfxAtlasId)
{
	auto circleConstantsResource = resourceManager->GetResource<ConstantBuffer>(circleConstantsId);
	auto haloConstantsResource = resourceManager->GetResource<ConstantBuffer>(haloConstantsId);
	auto perlinNoiseResource = resourceManager->GetResource<Texture>(perlinNoiseId);
	auto volumeNoiseResource = resourceManager->GetResource<Texture>(volumeNoiseId);
	auto turbulenceMapResource = resourceManager->GetResource<Texture>(turbulenceMapId);
	auto vfxAtlasResource = resourceManager->GetResource<Texture>(vfxAtlasId);
	auto haloSpectrumResource = resourceManager->GetResource<Texture>(vfxLuxHaloSpectrumId);
	auto samplerLinearResource = resourceManager->GetDefaultSampler(device,
		Graphics::DefaultFilterSetup::FILTER_TRILINEAR_WRAP);

	auto vfxLuxCircleVS = resourceManager->GetResource<Shader>(vfxLuxCircleVSId);
	auto vfxLuxHaloVS = resourceManager->GetResource<Shader>(vfxLuxHaloVSId);
	auto vfxLuxCirclePS = resourceManager->GetResource<Shader>(vfxLuxCirclePSId);
	auto vfxLuxHaloPS = resourceManager->GetResource<Shader>(vfxLuxHaloPSId);

	auto blendSetup = Graphics::DefaultBlendSetup::BLEND_TRANSPARENT;
	auto blendAddSetup = Graphics::DefaultBlendSetup::BLEND_PREMULT_ALPHA_ADDITIVE;

	MaterialBuilder materialBuilder{};
	materialBuilder.SetConstantBuffer(0u, circleConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, haloSpectrumResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, volumeNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(2u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(circleMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vfxLuxCircleVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxCirclePS->bytecode);

	circleMaterial = materialBuilder.ComposeStandard(device);

	materialBuilder.SetConstantBuffer(0u, haloConstantsResource->resourceGPUAddress);
	materialBuilder.SetTexture(0u, perlinNoiseResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetTexture(1u, haloSpectrumResource->srvDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetSampler(0u, samplerLinearResource->samplerDescriptor.gpuDescriptor, D3D12_SHADER_VISIBILITY_PIXEL);
	materialBuilder.SetCullMode(D3D12_CULL_MODE_NONE);
	materialBuilder.SetBlendMode(Graphics::DirectX12Utilities::CreateBlendDesc(blendSetup));
	materialBuilder.SetDepthStencilFormat(32u, false);
	materialBuilder.SetRenderTargetFormat(0u, DXGI_FORMAT_R16G16B16A16_FLOAT);
	materialBuilder.SetGeometryFormat(haloMesh->GetDesc().vertexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	materialBuilder.SetVertexShader(vfxLuxHaloVS->bytecode);
	materialBuilder.SetPixelShader(vfxLuxHaloPS->bytecode);

	haloMaterial = materialBuilder.ComposeStandard(device);
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
