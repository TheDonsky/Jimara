#pragma once
#include "../../Core/Object.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		// Forward declaration (to avoid circular dependencies...)
		class CommandBuffer;

		/// <summary> Arbitrary buffer </summary>
		class JIMARA_API Buffer : public virtual Object {
		public:
			/// <summary> CPU access flags </summary>
			enum class JIMARA_API CPUAccess : uint8_t {
				/// <summary> CPU can read and write </summary>
				CPU_READ_WRITE,

				/// <summary> CPU can only write </summary>
				CPU_WRITE_ONLY,

				/// <summary> 
				/// Usage with CPU is not straightforward 
				/// (mainly used with some backend-specific internal buffers and will you not encounter those kind in the wild) 
				/// </summary>
				OTHER
			};

			/// <summary> Virtual destructor </summary>
			inline virtual ~Buffer() {}

			/// <summary> Size of an individual object/structure within the buffer </summary>
			virtual size_t ObjectSize()const = 0;

			/// <summary> CPU access info </summary>
			virtual CPUAccess HostAccess()const = 0;

			/// <summary>
			/// Maps buffer memory to CPU
			/// <para/> Notes:
			///		<para/> 0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		<para/> 1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
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
			/// <returns> self </returns>
			inline BufferReference& operator=(Buffer* buffer) {
				assert(buffer == nullptr || buffer->ObjectSize() == sizeof(ObjectType));
				Reference<Buffer>::operator=(buffer); 
				return *this;
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
			/// <para/> Notes:
			///		<para/> 0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		<para/> 1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline ObjectType& Map()const {
				return *(static_cast<ObjectType*>(operator->()->Map()));
			}
		};


		/// <summary> Array-type buffer </summary>
		class JIMARA_API ArrayBuffer : public virtual Buffer {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~ArrayBuffer() {}

			/// <summary> Number of objects within the buffer </summary>
			virtual size_t ObjectCount()const = 0;

			/// <summary> Device address of the buffer for buffer_reference </summary>
			virtual uint64_t DeviceAddress()const = 0;

			/// <summary>
			/// Copies a region of given buffer into a region of this one
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record copy operation on </param>
			/// <param name="srcBuffer"> Source buffer </param>
			/// <param name="numBytes"> Number of bytes to copy (Element sizes do not have to match; this is size in bytes. if out of bounds size is requested, this number will be truncated) </param>
			/// <param name="dstOffset"> Index of the byte from this buffer to start writing at (Note: this indicates 'Byte', not element) </param>
			/// <param name="srcOffset"> Index of the byte from srcBuffer to start copying from (Note: this indicates 'Byte', not element) </param>
			virtual void Copy(CommandBuffer* commandBuffer, ArrayBuffer* srcBuffer, size_t numBytes = ~size_t(0), size_t dstOffset = 0u, size_t srcOffset = 0u) = 0;
		};


		/// <summary>
		/// Reference to a multi-element buffer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the stored structure </typeparam>
		template<typename ObjectType>
		class ArrayBufferReference : public Reference<ArrayBuffer> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="buffer"> Address </param>
			inline ArrayBufferReference(ArrayBuffer* buffer = nullptr) {
				(*this) = buffer;
			}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="buffer"> Address </param>
			inline ArrayBufferReference(const Reference<ArrayBuffer>& buffer) {
				(*this) = buffer.operator->();
			}

			/// <summary>
			/// Sets new address
			/// </summary>
			/// <param name="buffer"> New address </param>
			/// <returns> self </returns>
			inline ArrayBufferReference& operator=(ArrayBuffer* buffer) {
				assert(buffer == nullptr || buffer->ObjectSize() == sizeof(ObjectType));
				Reference<ArrayBuffer>::operator=(buffer);
				return *this;
			}

			/// <summary>
			/// Maps buffer memory to CPU
			/// <para/> Notes:
			///		<para/> 0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		<para/> 1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline ObjectType* Map()const {
				return static_cast<ObjectType*>(operator->()->Map());
			}
		};
	}
}
