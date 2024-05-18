#pragma once

#include "../DirectX12Includes.h"
#include "../VertexFormat.h"

namespace Graphics::Assets
{
	enum class IndexFormat : uint32_t
	{
		UINT16_INDEX = 0U,
		UINT32_INDEX = 1U
	};

	struct MeshDesc
	{
	public:
		VertexFormat vertexFormat;
		IndexFormat indexFormat;
		D3D12_PRIMITIVE_TOPOLOGY topology;
		uint32_t verticesNumber;
		uint32_t indicesNumber;
	};
}
