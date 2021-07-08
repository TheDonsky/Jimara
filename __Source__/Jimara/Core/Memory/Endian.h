#pragma once
#include <cstdint>


namespace Jimara {
	enum class Endian : uint8_t {
		LITTLE = 0,
		BIG = 1
	};

	inline static constexpr Endian NativeEndian() {
		uint32_t v = 1;
		return (((char*)&v)[0] == 1) ? Endian::LITTLE : Endian::BIG;
	}
}
