#pragma once
#include "../Object.h"
#include "Endian.h"
#include <cassert>



namespace Jimara {
	/// <summary>
	/// View on arbitrary memory block (externally allocated; read-only)
	/// </summary>
	class MemoryBlock : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="data"> View data </param>
		/// <param name="size"> Number of bytes in data </param>
		/// <param name="dataOwner"> Object to hold on to while the block is alive (general assumption is that data may go out of scope if the dataOwner gets deleted) </param>
		inline MemoryBlock(const void* data, size_t size, const Object* dataOwner) 
			: m_dataOwner(dataOwner), m_memory(data), m_size(size) {}

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Block to copy </param>
		inline MemoryBlock(const MemoryBlock& other) : m_dataOwner(other.m_dataOwner), m_memory(other.m_memory), m_size(other.m_size) {}

		/// <summary> View data </summary>
		inline const void* Data()const { return m_memory; }

		/// <summary> Number of bytes in data </summary>
		inline size_t Size()const { return m_size; }

		/// <summary> Object to hold on to while the block is alive (general assumption is that data may go out of scope if the dataOwner gets deleted) </summary>
		inline const Object* DataOwner()const { return m_dataOwner; }

		/// <summary>
		/// Retrieves value from binary data
		/// </summary>
		/// <typeparam name="ValueType"> Any of the primitive types (signed/unsigned 8/16/32/64 bit integers/floating points) </typeparam>
		/// <param name="data"> Binary data </param>
		/// <param name="endian"> Encoding endianness </param>
		/// <returns> Value from data </returns>
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

		/// <summary>
		/// Retrieves value from binary data
		/// </summary>
		/// <typeparam name="ValueType"> Any of the primitive types (signed/unsigned 8/16/32/64 bit integers/floating points) </typeparam>
		/// <param name="offset"> Iterator/Offset from origin (will be auto-incremented by sizeof(ValueType)) </param>
		/// <param name="endian"> Encoding endianness </param>
		/// <returns> Value from data at given offset </returns>
		template<typename ValueType>
		inline ValueType Get(size_t& offset, Endian endian = NativeEndian())const {
			assert((offset + sizeof(ValueType)) <= Size());
			const void* data = reinterpret_cast<const void*>(reinterpret_cast<const char*>(Data()) + offset);
			offset += sizeof(ValueType);
			return Get<ValueType>(data, endian);
		}


	private:
		// Object to hold on to while the block is alive (general assumption is that data may go out of scope if the dataOwner gets deleted)
		Reference<const Object> m_dataOwner;

		// View data
		const void* m_memory;

		// Number of bytes in data
		size_t m_size;

		// Copy is disabled
		inline MemoryBlock& operator=(const MemoryBlock&) = delete;
	};



	/// <summary>
	/// View on arbitrary memory block (externally allocated; read-write)
	/// </summary>
	class MemoryBlockRW : public virtual MemoryBlock {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="data"> View data </param>
		/// <param name="size"> Number of bytes in data </param>
		/// <param name="dataOwner"> Object to hold on to while the block is alive (general assumption is that data may go out of scope if the dataOwner gets deleted) </param>
		inline MemoryBlockRW(void* data, size_t size, const Object* dataOwner)
			: MemoryBlock(data, size, dataOwner), m_memoryRW(data) {}

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Block to copy </param>
		inline MemoryBlockRW(const MemoryBlockRW& other) : MemoryBlock(other), m_memoryRW(other.m_memoryRW) {}

		/// <summary> View data </summary>
		inline void* Data() { return m_memoryRW; }

		/// <summary>
		/// Encodes value as binary data
		/// </summary>
		/// <typeparam name="ValueType"> Any of the primitive types (signed/unsigned 8/16/32/64 bit integers/floating points) </typeparam>
		/// <param name="data"> Binary(destination) data </param>
		/// <param name="value"> Value to store </param>
		/// <param name="endian"> Encoding endianness </param>
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

		/// <summary>
		/// Encodes value as binary data
		/// </summary>
		/// <typeparam name="ValueType"> Any of the primitive types (signed/unsigned 8/16/32/64 bit integers/floating points) </typeparam>
		/// <param name="offset"> Iterator/Offset from origin (will be auto-incremented by sizeof(ValueType)) </param>
		/// <param name="value"> Value to store </param>
		/// <param name="endian"> Encoding endianness </param>
		template<typename ValueType>
		inline void Set(size_t& offset, const ValueType& value, Endian endian = NativeEndian()) {
			assert((offset + sizeof(ValueType)) <= Size());
			const void* data = reinterpret_cast<const void*>(reinterpret_cast<const char*>(Data()) + offset);
			offset += sizeof(ValueType);
			Set<ValueType>(data, value, endian);
		}

	private:
		// Write-enabled copy of Data()
		void* m_memoryRW;
	};
}
