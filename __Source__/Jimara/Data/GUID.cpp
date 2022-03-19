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
			while (true) {
				bytes[i] = rng();
				if (bytes[i] != 0) break;
			}
		return id;
	}

	bool GUID::operator<(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) < 0; }

	bool GUID::operator<=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) <= 0; }

	bool GUID::operator==(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) == 0; }

	bool GUID::operator!=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) != 0; }

	bool GUID::operator>=(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) >= 0; }

	bool GUID::operator>(const GUID& other)const { return memcmp(bytes, other.bytes, NUM_BYTES) > 0; }

	GUID::Serializer::Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes) 
		: ItemSerializer(name, hint, attributes) {}


	namespace {
		static_assert((Jimara::GUID::NUM_BYTES % sizeof(uint64_t)) == 0);
		static const constexpr uint64_t GUID_WORD_COUNT = (Jimara::GUID::NUM_BYTES / sizeof(uint64_t));

	}

	void GUID::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, GUID* target)const {
		if (target == nullptr) {
			GUID zero = {};
			GetFields(recordElement, &zero);
			return;
		}
		static_assert(sizeof(uint64_t) == sizeof(int64_t));
		int64_t* data = reinterpret_cast<int64_t*>(target->bytes);
		static const Reference<const Serialization::ItemSerializer::Of<int64_t>> wordSerializer = Serialization::ValueSerializer<int64_t>::Create("Word", "GUID word");
		for (size_t i = 0; i < GUID_WORD_COUNT; i++)
			recordElement(wordSerializer->Serialize(data + i));
	}

	std::ostream& operator<<(std::ostream& stream, const GUID& guid) {
		const uint64_t* data = reinterpret_cast<const uint64_t*>(guid.bytes);
		for (size_t i = 0; i < GUID_WORD_COUNT; i++)
			stream << ((i == 0) ? "{" : " - ") << data[i];
		stream << '}';
		return stream;
	}

	std::istream& operator>>(std::istream& stream, GUID& guid) {
		uint64_t* data = reinterpret_cast<uint64_t*>(guid.bytes);
		char c;
		for (size_t i = 0; i < GUID_WORD_COUNT; i++) {
			for (size_t j = ((i == 0) ? 1 : 3); j > 0; j--)
				stream.get(c);
			stream >> data[i];
		}
		stream.get(c);
		return stream;
	}
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
