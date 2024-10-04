#pragma once

#include "../../../Includes.h"
#include "RaytracingShaderRecord.h"

namespace Graphics::Assets::Raytracing
{
	class RaytracingShaderTable
	{
	public:
		RaytracingShaderTable();
		~RaytracingShaderTable();

		void AddRecord(RaytracingShaderRecord&& record, RaytracingShaderRecordKind recordKind);
		
		std::vector<uint8_t> Compose();

		size_t GetRayGenerationShaderRecordSize() const;
		size_t GetHitGroupShaderRecordSize() const;
		size_t GetMissShaderRecordSize() const;

		size_t GetRayGenerationShaderTableSize() const;
		size_t GetHitGroupTableSize() const;
		size_t GetMissTableSize() const;

		size_t GetSize() const;

	private:
		size_t AlignSize(size_t size, size_t alignment) const;

		RaytracingShaderRecord rayGenerationShaderRecord;
		std::vector<RaytracingShaderRecord> hitGroupShaderRecords;
		std::vector<RaytracingShaderRecord> missShaderRecords;

		size_t hitGroupShaderRecordStride;
		size_t missShaderRecordStride;
	};
}
