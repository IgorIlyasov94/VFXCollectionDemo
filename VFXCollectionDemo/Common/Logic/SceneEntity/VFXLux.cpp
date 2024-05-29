#include "VFXLux.h"

using namespace Graphics;
using namespace Graphics::Assets;
using namespace Graphics::Resources;
using namespace DirectX::PackedVector;

Common::Logic::SceneEntity::VFXLux::VFXLux(ID3D12GraphicsCommandList* commandList, DirectX12Renderer* renderer,
	ResourceID vfxAtlasId)
{
	auto device = renderer->GetDevice();
	auto resourceManager = renderer->GetResourceManager();

	CreateMesh(device, commandList, resourceManager);
}

Common::Logic::SceneEntity::VFXLux::~VFXLux()
{

}

void Common::Logic::SceneEntity::VFXLux::Draw(ID3D12GraphicsCommandList* commandList)
{

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
		uint8_t r = static_cast<uint8_t>((1.0f - rFloat * rFloat) * 255.0f);

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

		uint8_t r = static_cast<uint8_t>((1.0f - rFloat * rFloat) * 255.0f);
		uint8_t g = static_cast<uint8_t>((1.0f - gFloat * gFloat) * 255.0f);
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
		uint8_t r = static_cast<uint8_t>((1.0f - rFloat * rFloat) * 255.0f);

		vertex.color = SetColor(r, 0u, 0u, 0u);
	}

	uint16_t indices[INDICES_NUMBER]{};

	for (uint32_t index = 0; index < INDICES_NUMBER; index++)
	{
		auto& vertexIndex = indices[index];

		auto localIndex = index % INDICES_PER_TRIANGLE;
		auto rowIndex = index / INDICES_PER_TRIANGLE;
		auto segmentIndex = index / INDICES_PER_SEGMENT;

		vertexIndex = INDEX_OFFSETS[localIndex] + rowIndex + segmentIndex;
	}

	BufferDesc vertexBufferDesc(&vertices, sizeof(vertices));

	auto vertexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::VERTEX_BUFFER, vertexBufferDesc);

	BufferDesc indexBufferDesc(&indices, sizeof(indices));

	auto indexBufferId = resourceManager->CreateBufferResource(device, commandList,
		BufferResourceType::INDEX_BUFFER, indexBufferDesc);

	MeshDesc meshDesc{};
	meshDesc.verticesNumber = VERTICES_NUMBER;
	meshDesc.indicesNumber = INDICES_NUMBER;
	meshDesc.vertexFormat = VertexFormat::POSITION | VertexFormat::TEXCOORD0 | VertexFormat::COLOR0;
	meshDesc.indexFormat = IndexFormat::UINT16_INDEX;
	meshDesc.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	
	vfxMesh = new Mesh(meshDesc, vertexBufferId, indexBufferId, resourceManager);
}

void Common::Logic::SceneEntity::VFXLux::CreateMaterials(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	Graphics::Resources::ResourceManager* resourceManager)
{

}

uint32_t Common::Logic::SceneEntity::VFXLux::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return static_cast<uint32_t>(r) << 24u | static_cast<uint32_t>(g) << 16u |
		static_cast<uint32_t>(b) << 8u | static_cast<uint32_t>(a);
}
