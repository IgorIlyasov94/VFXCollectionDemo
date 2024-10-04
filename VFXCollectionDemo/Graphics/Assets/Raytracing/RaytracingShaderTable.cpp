#include "RaytracingShaderTable.h"
#include "../../DirectX12Includes.h"

Graphics::Assets::Raytracing::RaytracingShaderTable::RaytracingShaderTable()
	: rayGenerationShaderRecord{}, hitGroupShaderRecordStride(0ull), missShaderRecordStride(0ull)
{

}

Graphics::Assets::Raytracing::RaytracingShaderTable::~RaytracingShaderTable()
{

}

void Graphics::Assets::Raytracing::RaytracingShaderTable::AddRecord(RaytracingShaderRecord&& record, RaytracingShaderRecordKind recordKind)
{
	if (recordKind == RaytracingShaderRecordKind::RAY_GENERATION_SHADER_RECORD)
		rayGenerationShaderRecord = std::forward<RaytracingShaderRecord>(record);
	else if (recordKind == RaytracingShaderRecordKind::HIT_GROUP_SHADER_RECORD)
	{
		hitGroupShaderRecordStride = std::max(hitGroupShaderRecordStride, record.GetSize());
		hitGroupShaderRecords.push_back(std::forward<RaytracingShaderRecord>(record));
	}
	else
	{
		missShaderRecordStride = std::max(missShaderRecordStride, record.GetSize());
		missShaderRecords.push_back(std::forward<RaytracingShaderRecord>(record));
	}
}

std::vector<uint8_t> Graphics::Assets::Raytracing::RaytracingShaderTable::Compose()
{
	std::vector<uint8_t> composedData;

	auto tableSize = GetSize();
	composedData.resize(tableSize, 0u);

	auto startAddress = rayGenerationShaderRecord.GetData();
	auto endAddress = startAddress + GetRayGenerationShaderRecordSize();
	auto destAddress = composedData.data();

	std::copy(startAddress, endAddress, destAddress);
	destAddress += GetRayGenerationShaderTableSize();

	for (auto& shaderRecord : missShaderRecords)
	{
		startAddress = shaderRecord.GetData();
		endAddress = startAddress + shaderRecord.GetSize();

		std::copy(startAddress, endAddress, destAddress);
		destAddress += missShaderRecordStride;
	}

	destAddress = composedData.data();
	destAddress += GetRayGenerationShaderTableSize();
	destAddress += GetMissTableSize();

	for (auto& shaderRecord : hitGroupShaderRecords)
	{
		startAddress = shaderRecord.GetData();
		endAddress = startAddress + shaderRecord.GetSize();

		std::copy(startAddress, endAddress, destAddress);
		destAddress += hitGroupShaderRecordStride;
	}

	return composedData;
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetRayGenerationShaderRecordSize() const
{
	return rayGenerationShaderRecord.GetSize();
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetHitGroupShaderRecordSize() const
{
	return hitGroupShaderRecordStride;
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetMissShaderRecordSize() const
{
	return missShaderRecordStride;
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetRayGenerationShaderTableSize() const
{
	return AlignSize(rayGenerationShaderRecord.GetSize(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetHitGroupTableSize() const
{
	return AlignSize(hitGroupShaderRecordStride * hitGroupShaderRecords.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetMissTableSize() const
{
	return AlignSize(missShaderRecordStride * missShaderRecords.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::GetSize() const
{
	auto tableSize = GetRayGenerationShaderTableSize();
	tableSize += GetHitGroupTableSize();
	tableSize += GetMissTableSize();

	return tableSize;
}

size_t Graphics::Assets::Raytracing::RaytracingShaderTable::AlignSize(size_t size, size_t alignment) const
{
	return (size + alignment - 1) & ~(alignment - 1);
}
