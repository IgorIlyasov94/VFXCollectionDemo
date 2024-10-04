#include "RaytracingShaderRecord.h"
#include "../../DirectX12Includes.h"

Graphics::Assets::Raytracing::RaytracingShaderRecord::RaytracingShaderRecord()
{

}

Graphics::Assets::Raytracing::RaytracingShaderRecord::~RaytracingShaderRecord()
{

}

Graphics::Assets::Raytracing::RaytracingShaderRecord::RaytracingShaderRecord(const RaytracingShaderRecord& newShaderRecord)
	: recordData(newShaderRecord.recordData)
{

}

Graphics::Assets::Raytracing::RaytracingShaderRecord::RaytracingShaderRecord(RaytracingShaderRecord&& newShaderRecord) noexcept
	: recordData(newShaderRecord.recordData)
{

}

Graphics::Assets::Raytracing::RaytracingShaderRecord& Graphics::Assets::Raytracing::RaytracingShaderRecord::operator=(const RaytracingShaderRecord& newShaderRecord)
{
	if (this != &newShaderRecord)
		recordData = newShaderRecord.recordData;

	return *this;
}

Graphics::Assets::Raytracing::RaytracingShaderRecord& Graphics::Assets::Raytracing::RaytracingShaderRecord::operator=(RaytracingShaderRecord&& newShaderRecord) noexcept
{
	if (this != &newShaderRecord)
		recordData = newShaderRecord.recordData;

	return *this;
}

void Graphics::Assets::Raytracing::RaytracingShaderRecord::AddData(const void* data, size_t size)
{
	auto currentSize = recordData.size();
	recordData.resize(currentSize + size, 0u);

	auto startAddress = reinterpret_cast<const uint8_t*>(data);
	auto endAddress = startAddress + size;
	auto destAddress = recordData.data() + currentSize;

	std::copy(startAddress, endAddress, destAddress);
}

const uint8_t* Graphics::Assets::Raytracing::RaytracingShaderRecord::GetData() const
{
	return recordData.data();
}

size_t Graphics::Assets::Raytracing::RaytracingShaderRecord::GetSize() const
{
	return (recordData.size() + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) & ~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);
}
