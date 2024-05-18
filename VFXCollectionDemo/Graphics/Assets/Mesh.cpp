#include "Mesh.h"
#include "Loaders/OBJLoader.h"

using namespace Graphics::Resources;
using namespace Graphics::Assets::Loaders;

Graphics::Assets::Mesh::Mesh(const MeshDesc& meshDesc, ResourceID vertexBufferId, ResourceID indexBufferId,
	ResourceManager* resourceManager)
	: _vertexBufferId(vertexBufferId), _indexBufferId(indexBufferId), topology(meshDesc.topology),
	indicesNumber(meshDesc.indicesNumber)
{
	auto vertexBuffer = resourceManager->GetResource<VertexBuffer>(_vertexBufferId);
	auto indexBuffer = resourceManager->GetResource<IndexBuffer>(_indexBufferId);

	vertexBufferView = &vertexBuffer->viewDesc;
	indexBufferView = &indexBuffer->viewDesc;
}

Graphics::Assets::Mesh::Mesh(std::filesystem::path filePath, ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ResourceManager* resourceManager, bool recalculateNormals)
{
	MeshDesc meshDesc{};

	BufferDesc vbDesc{};
	BufferDesc ibDesc{};

	if (filePath.extension() == ".obj" || filePath.extension() == ".OBJ")
		OBJLoader::Load(filePath, recalculateNormals, meshDesc, vbDesc.data, ibDesc.data);

	vbDesc.dataStride = static_cast<uint32_t>(vbDesc.data.size() / meshDesc.verticesNumber);
	ibDesc.dataStride = static_cast<uint32_t>(ibDesc.data.size() / meshDesc.indicesNumber);
	ibDesc.numElements = meshDesc.indicesNumber;
	
	_vertexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::VERTEX_BUFFER, vbDesc);
	_indexBufferId = resourceManager->CreateBufferResource(device, commandList, BufferResourceType::INDEX_BUFFER, ibDesc);

	auto vertexBuffer = resourceManager->GetResource<VertexBuffer>(_vertexBufferId);
	auto indexBuffer = resourceManager->GetResource<IndexBuffer>(_indexBufferId);

	vertexBufferView = &vertexBuffer->viewDesc;
	indexBufferView = &indexBuffer->viewDesc;
	indicesNumber = meshDesc.indicesNumber;
	topology = meshDesc.topology;
}

Graphics::Assets::Mesh::~Mesh()
{

}

void Graphics::Assets::Mesh::Release(Resources::ResourceManager* resourceManager)
{
	resourceManager->DeleteResource<VertexBuffer>(_vertexBufferId);
	resourceManager->DeleteResource<IndexBuffer>(_indexBufferId);
}

void Graphics::Assets::Mesh::Draw(ID3D12GraphicsCommandList* commandList)
{
	commandList->IASetPrimitiveTopology(topology);
	commandList->IASetVertexBuffers(0u, 1u, vertexBufferView);
	commandList->IASetIndexBuffer(indexBufferView);

	commandList->DrawIndexedInstanced(indicesNumber, 1, 0, 0, 0);
}
