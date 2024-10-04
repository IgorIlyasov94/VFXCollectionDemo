#pragma once

#include "../../../Includes.h"

namespace Graphics::Assets::Raytracing
{
	enum class RaytracingShaderRecordKind : uint32_t
	{
		RAY_GENERATION_SHADER_RECORD = 0u,
		HIT_GROUP_SHADER_RECORD = 1u,
		MISS_SHADER_RECORD = 2u
	};

	class RaytracingShaderRecord
	{
	public:
		RaytracingShaderRecord();
		~RaytracingShaderRecord();

		RaytracingShaderRecord(const RaytracingShaderRecord& newShaderRecord);
		RaytracingShaderRecord(RaytracingShaderRecord&& newShaderRecord) noexcept;
		RaytracingShaderRecord& operator=(const RaytracingShaderRecord& newShaderRecord);
		RaytracingShaderRecord& operator=(RaytracingShaderRecord&& newShaderRecord) noexcept;

		void AddData(const void* data, size_t size);

		const uint8_t* GetData() const;
		size_t GetSize() const;

	private:
		std::vector<uint8_t> recordData;
	};
}
