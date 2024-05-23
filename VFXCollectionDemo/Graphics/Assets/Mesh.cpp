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

	std::filesystem::path filePathCache(filePath);
	filePathCache.replace_filename(filePath.filename().generic_string() + "CACHE");
	auto loadCache = std::filesystem::exists(filePathCache);

	if (loadCache)
	{
		auto mainFileTimestamp = std::filesystem::last_write_time(filePath);
		auto cacheFileTimestamp = std::filesystem::last_write_time(filePathCache);

		loadCache = cacheFileTimestamp >= mainFileTimestamp;
	}

	if (loadCache)
	{
		LoadCache(filePathCache, meshDesc, vbDesc.data, ibDesc.data);
	}
	else
	{
		if (filePath.extension() == ".obj" || filePath.extension() == ".OBJ")
			OBJLoader::Load(filePath, recalculateNormals, meshDesc, vbDesc.data, ibDesc.data);

		SaveCache(filePathCache, meshDesc, vbDesc.data, ibDesc.data);
	}

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

void Graphics::Assets::Mesh::LoadCache(std::filesystem::path filePath, MeshDesc& meshDesc,
	std::vector<uint8_t>& verticesData, std::vector<uint8_t>& indicesData)
{
	std::ifstream meshFile(filePath, std::ios::binary);
	meshFile.read(reinterpret_cast<char*>(&meshDesc), sizeof(MeshDesc));

	auto vertexBufferSize = static_cast<size_t>(meshDesc.verticesNumber) * VertexStride(meshDesc.vertexFormat);
	verticesData.resize(vertexBufferSize);
	meshFile.read(reinterpret_cast<char*>(verticesData.data()), vertexBufferSize);

	auto indexBufferSize = static_cast<size_t>(meshDesc.indicesNumber);
	indexBufferSize *= meshDesc.indexFormat == IndexFormat::UINT16_INDEX ? 2u : 4u;
	indicesData.resize(indexBufferSize);
	meshFile.read(reinterpret_cast<char*>(indicesData.data()), indexBufferSize);
}

void Graphics::Assets::Mesh::SaveCache(std::filesystem::path filePath, const MeshDesc& meshDesc,
	const std::vector<uint8_t>& verticesData, const std::vector<uint8_t>& indicesData)
{
	std::ofstream meshFile(filePath, std::ios::binary);
	meshFile.write(reinterpret_cast<const char*>(&meshDesc), sizeof(MeshDesc));
	meshFile.write(reinterpret_cast<const char*>(verticesData.data()), verticesData.size());
	meshFile.write(reinterpret_cast<const char*>(indicesData.data()), indicesData.size());
}
