#pragma once

#include "../DirectX12Includes.h"
#include "../VertexFormat.h"
#include "../Resources/ResourceManager.h"
#include "MeshDesc.h"

namespace Graphics::Assets
{
	class Mesh
	{
	public:
		Mesh(const MeshDesc& meshDesc, Resources::ResourceID vertexBufferId, Resources::ResourceID indexBufferId,
			Resources::ResourceManager* resourceManager);
		Mesh(std::filesystem::path filePath, ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
			Resources::ResourceManager* resourceManager, bool recalculateNormals);
		~Mesh();

		void Release(Resources::ResourceManager* resourceManager);

		void Draw(ID3D12GraphicsCommandList* commandList);
		
	private:
		Mesh() = delete;

		D3D12_VERTEX_BUFFER_VIEW* vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW* indexBufferView;

		Resources::ResourceID _vertexBufferId;
		Resources::ResourceID _indexBufferId;

		uint32_t indicesNumber;
		D3D12_PRIMITIVE_TOPOLOGY topology;
	};
}
