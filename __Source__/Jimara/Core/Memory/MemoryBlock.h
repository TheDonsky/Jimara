#pragma once
#include "../Object.h"
#include "Endian.h"
#include <cassert>



namespace Jimara {
	class MemoryBlock : public virtual Object {
	public:
		inline MemoryBlock(const void* data, size_t size, const Object* dataOwner) 
			: m_dataOwner(dataOwner), m_memory(data), m_size(size) {}

		inline MemoryBlock(const MemoryBlock& other) : m_dataOwner(other.m_dataOwner), m_memory(other.m_memory), m_size(other.m_size) {}

		inline MemoryBlock& operator=(const MemoryBlock&) = delete;

		inline const void* Data()const { return m_memory; }

		inline size_t Size()const { return m_size; }

		inline const Object* DataOwner()const { return m_dataOwner; }

		template<typename ValueType>
		inline static ValueType Get(const void* data, Endian endian = NativeEndian()) {
			if (endian == NativeEndian()) return *reinterpret_cast<const ValueType*>(data);
			else {
				char invData[sizeof(ValueType)];
				const char* cData = reinterpret_cast<const char*>(data) + sizeof(ValueType) - 1;
				for (size_t i = 0; i < sizeof(ValueType); i++)
					invData[i] = *(cData - i);
				return *reinterpret_cast<const ValueType*>(invData);
			}
		}

		template<typename ValueType>
		inline ValueType Get(size_t& offset, Endian endian = NativeEndian())const {
			assert((offset + sizeof(ValueType)) <= Size());
			const void* data = reinterpret_cast<const void*>(reinterpret_cast<const char*>(Data()) + offset);
			offset += sizeof(ValueType);
			return Get<ValueType>(data, endian);
		}


	private:
		Reference<const Object> m_dataOwner;
		const void* m_memory;
		size_t m_size;
	};

	class MemoryBlockRW : public virtual MemoryBlock {
	public:
		inline MemoryBlockRW(void* data, size_t size, const Object* dataOwner)
			: MemoryBlock(data, size, dataOwner), m_memoryRW(data) {}

		inline MemoryBlockRW(const MemoryBlockRW& other) : MemoryBlock(other), m_memoryRW(other.m_memoryRW) {}

		inline void* Data() { return m_memoryRW; }

		template<typename ValueType>
		inline static void Set(void* data, const ValueType& value, Endian endian = NativeEndian()) {
			if (endian == NativeEndian()) (*reinterpret_cast<ValueType*>(data)) = value;
			else {
				char* invData = reinterpret_cast<char*>(data);
				const char* cData = reinterpret_cast<char*>(&value) + sizeof(ValueType) - 1;
				for (size_t i = 0; i < sizeof(ValueType); i++)
					invData[i] = *(cData - i);
			}
		}

		template<typename ValueType>
		inline void Set(size_t& offset, const ValueType& value, Endian endian = NativeEndian()) {
			assert((offset + sizeof(ValueType)) <= Size());
			const void* data = reinterpret_cast<const void*>(reinterpret_cast<const char*>(Data()) + offset);
			offset += sizeof(ValueType);
			Set<ValueType>(data, value, endian);
		}

	private:
		void* m_memoryRW;
	};
}
