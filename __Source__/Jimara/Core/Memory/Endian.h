#pragma once
#include <cstdint>


namespace Jimara {
	/// <summary>
	/// Indicates the endianness of scalar types
	/// </summary>
	enum class Endian : uint8_t {
		/// <summary> Little-endian </summary>
		LITTLE = 0,

		/// <summary> Big-endian </summary>
		BIG = 1
	};

	/// <summary> Tells if the syatem is big-endian or little-endian </summary>
	inline static constexpr Endian NativeEndian() {
		uint32_t v = 1;
		return (((char*)&v)[0] == 1) ? Endian::LITTLE : Endian::BIG;
	}
}
