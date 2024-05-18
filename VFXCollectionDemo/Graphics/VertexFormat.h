#pragma once

#include "../Includes.h"

namespace Graphics
{
	enum class VertexFormat : uint32_t
	{
		UNDEFINED = 0u,
		POSITION = 1u,
		NORMAL = POSITION << 1u,
		TANGENT = POSITION << 2u,
		COLOR0 = POSITION << 3u,
		COLOR1 = POSITION << 4u,
		COLOR2 = POSITION << 5u,
		COLOR3 = POSITION << 6u,
		COLOR4 = POSITION << 7u,
		COLOR5 = POSITION << 8u,
		COLOR6 = POSITION << 9u,
		COLOR7 = POSITION << 10u,
		TEXCOORD0 = POSITION << 11u,
		TEXCOORD1 = POSITION << 12u,
		TEXCOORD2 = POSITION << 13u,
		TEXCOORD3 = POSITION << 14u,
		TEXCOORD4 = POSITION << 15u,
		TEXCOORD5 = POSITION << 16u,
		TEXCOORD6 = POSITION << 17u,
		TEXCOORD7 = POSITION << 18u,
		BLENDINDICES = POSITION << 19u,
		BLENDWEIGHT = POSITION << 20u
	};

	inline VertexFormat operator|(const VertexFormat& leftValue, const VertexFormat& rightValue)
	{
		using UnderlyingType = std::underlying_type_t<VertexFormat>;

		auto format = static_cast<UnderlyingType>(leftValue);
		format |= static_cast<UnderlyingType>(rightValue);

		return static_cast<VertexFormat>(format);
	}

	inline VertexFormat& operator|=(VertexFormat& leftValue, const VertexFormat& rightValue)
	{
		leftValue = leftValue | rightValue;

		return leftValue;
	}

	inline VertexFormat operator&(const VertexFormat& leftValue, const VertexFormat& rightValue)
	{
		using UnderlyingType = std::underlying_type_t<VertexFormat>;

		auto format = static_cast<UnderlyingType>(leftValue);
		format &= static_cast<UnderlyingType>(rightValue);

		return static_cast<VertexFormat>(format);
	}

	inline VertexFormat& operator&=(VertexFormat& leftValue, const VertexFormat& rightValue)
	{
		leftValue = leftValue & rightValue;

		return leftValue;
	}

	inline VertexFormat operator~(const VertexFormat& value)
	{
		using UnderlyingType = std::underlying_type_t<VertexFormat>;

		auto format = ~static_cast<UnderlyingType>(value);

		return static_cast<VertexFormat>(format);
	}

	inline VertexFormat& operator~(VertexFormat& value)
	{
		using UnderlyingType = std::underlying_type_t<VertexFormat>;

		auto format = ~static_cast<UnderlyingType>(value);
		value = static_cast<VertexFormat>(format);

		return value;
	}

	constexpr uint32_t VertexStride(const VertexFormat& format)
	{
		uint32_t result = 0u;
		result += (format & VertexFormat::POSITION) == VertexFormat::POSITION ? 12u : 0u;
		result += (format & VertexFormat::NORMAL) == VertexFormat::NORMAL ? 8u : 0u;
		result += (format & VertexFormat::TANGENT) == VertexFormat::TANGENT ? 8u : 0u;
		result += (format & VertexFormat::COLOR0) == VertexFormat::COLOR0 ? 4u : 0u;
		result += (format & VertexFormat::COLOR1) == VertexFormat::COLOR1 ? 4u : 0u;
		result += (format & VertexFormat::COLOR2) == VertexFormat::COLOR2 ? 4u : 0u;
		result += (format & VertexFormat::COLOR3) == VertexFormat::COLOR3 ? 4u : 0u;
		result += (format & VertexFormat::COLOR4) == VertexFormat::COLOR4 ? 4u : 0u;
		result += (format & VertexFormat::COLOR5) == VertexFormat::COLOR5 ? 4u : 0u;
		result += (format & VertexFormat::COLOR6) == VertexFormat::COLOR6 ? 4u : 0u;
		result += (format & VertexFormat::COLOR7) == VertexFormat::COLOR7 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD0) == VertexFormat::TEXCOORD0 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD1) == VertexFormat::TEXCOORD1 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD2) == VertexFormat::TEXCOORD2 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD3) == VertexFormat::TEXCOORD3 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD4) == VertexFormat::TEXCOORD4 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD5) == VertexFormat::TEXCOORD5 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD6) == VertexFormat::TEXCOORD6 ? 4u : 0u;
		result += (format & VertexFormat::TEXCOORD7) == VertexFormat::TEXCOORD7 ? 4u : 0u;
		result += (format & VertexFormat::BLENDINDICES) == VertexFormat::BLENDINDICES ? 4u : 0u;
		result += (format & VertexFormat::BLENDWEIGHT) == VertexFormat::BLENDWEIGHT ? 8u : 0u;

		return result;
	}
}
