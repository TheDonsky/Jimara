#include "GUID.h"
#include "../Math/Random.h"
#include "../Math/Helpers.h"
#include <cstring>


namespace Jimara {
	GUID GUID::Generate() {
		// __TODO__: Maybe... use some fancy-ass algorithm for proper GUID generation instead of a simple RNG...
		GUID id;
		static_assert((NUM_BYTES % sizeof(std::mt19937::result_type)) == 0);
		const size_t COUNT = (NUM_BYTES / sizeof(std::mt19937::result_type));
		std::mt19937& rng = Random::ThreadRNG();
		std::mt19937::result_type* bytes = reinterpret_cast<std::mt19937::result_type*>(id.bytes);
		for (size_t i = 0; i < COUNT; i++)
			bytes[i] = rng();
		return id;
	}

	bool GUID::operator<(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) < 0; }

	bool GUID::operator<=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) <= 0; }

	bool GUID::operator==(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) == 0; }

	bool GUID::operator!=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) != 0; }

	bool GUID::operator>=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) >= 0; }

	bool GUID::operator>(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) > 0; }
}

namespace std {
	size_t hash<Jimara::GUID>::operator()(const Jimara::GUID& guid)const {
		static_assert((Jimara::GUID::NUM_BYTES % sizeof(size_t)) == 0);
		const size_t COUNT = (Jimara::GUID::NUM_BYTES / sizeof(size_t));
		static_assert(COUNT > 0);
		
		const size_t* data = reinterpret_cast<const size_t*>(guid.bytes);
		const size_t* end = (data + COUNT);
		
		hash<size_t> subHash;
		size_t result = subHash(*data);
		while (true) {
			data++;
			if (data >= end) return result;
			else result = Jimara::MergeHashes(result, subHash(*data));
		}
	}
}
