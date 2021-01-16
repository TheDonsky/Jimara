#pragma once
#include "../../Core/Object.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		/// <summary> Arbitrary buffer </summary>
		class Buffer : public virtual Object {
		public:
			/// <summary> CPU access flags </summary>
			enum class CPUAccess : uint8_t {
				/// <summary> CPU can read and write </summary>
				CPU_READ_WRITE,

				/// <summary> CPU can only write </summary>
				CPU_WRITE_ONLY
			};

			/// <summary> Virtual destructor </summary>
			inline virtual ~Buffer() {}

			/// <summary> Size of an individual object/structure within the buffer </summary>
			virtual size_t ObjectSize()const = 0;

			/// <summary> CPU access info </summary>
			virtual CPUAccess HostAccess()const = 0;

			/// <summary>
			/// Maps buffer memory to CPU
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			virtual void* Map() = 0;

			/// <summary>
			/// Unmaps memory previously mapped via Map() call
			/// </summary>
			/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
			virtual void Unmap(bool write) = 0;
		};


		/// <summary>
		/// Reference to a single element buffer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the stored structure </typeparam>
		template<typename ObjectType>
		class BufferReference : public Reference<Buffer> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="buffer"> Address </param>
			inline BufferReference(Buffer* buffer = nullptr) {
				(*this) = buffer;
			}

			/// <summary>
			/// Sets new address
			/// </summary>
			/// <param name="buffer"> New address </param>
			inline operator=(Buffer* buffer) { 
				assert(buffer == nullptr || buffer->ObjectSize() == sizeof(ObjectType));
				Reference<Buffer>::operator=(buffer); 
			}

			/// <summary> Type cast to underlying structure </summary>
			inline operator ObjectType ()const { 
				ObjectType value = *(static_cast<ObjectType*>(operator->()->Map()));
				operator->()->Unmap(false);
				return value;
			}

			/// <summary> Sets content of the single element buffer </summary>
			inline BufferReference& operator=(const ObjectType& value)const {
				(*(static_cast<ObjectType*>(operator->()->Map()))) = value;
				operator->()->Unmap(true);
				return *this;
			}

			/// <summary>
			/// Maps buffer memory to CPU
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline BufferReference& Map()const {
				return *(static_cast<ObjectType*>(operator->()->Map()));
			}
		};


		/// <summary> Array-type buffer </summary>
		class ArrayBuffer : public virtual Buffer {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~ArrayBuffer() {}

			/// <summary> Number of objects within the buffer </summary>
			virtual size_t ObjectCount()const = 0;
		};


		/// <summary>
		/// Reference to a multi-element buffer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the stored structure </typeparam>
		template<typename ObjectType>
		class BufferArrayReference : public Reference<ArrayBuffer> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="buffer"> Address </param>
			inline BufferArrayReference(ArrayBuffer* buffer = nullptr) {
				(*this) = buffer;
			}

			/// <summary>
			/// Sets new address
			/// </summary>
			/// <param name="buffer"> New address </param>
			inline operator=(ArrayBuffer* buffer) {
				assert(buffer == nullptr || buffer->ObjectSize() == sizeof(ObjectType));
				Reference<ArrayBuffer>::operator=(buffer);
			}

			/// <summary>
			/// Maps buffer memory to CPU
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline BufferReference* Map()const {
				return static_cast<ObjectType*>(operator->()->Map());
			}
		};
	}
}
