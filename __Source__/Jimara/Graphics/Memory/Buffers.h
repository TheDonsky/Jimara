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
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline ObjectType& Map()const {
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
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline ObjectType* Map()const {
				return static_cast<ObjectType*>(operator->()->Map());
			}
		};



		/// <summary>
		/// Vertex/Instance buffer interface
		/// </summary>
		class VertexBuffer : public virtual Object {
		public:
			/// <summary> Buffer attribute description </summary>
			struct AttributeInfo {
				/// <summary> Attribute type </summary>
				enum class Type : uint8_t {
					/// <summary> Single precision (32 bit) floating point </summary>
					FLOAT = 0,

					/// <summary> Single precision (32 bit) 2d floating point vector </summary>
					FLOAT2 = 1,

					/// <summary> Single precision (32 bit) 3d floating point vector </summary>
					FLOAT3 = 2,

					/// <summary> Single precision (32 bit) 4d floating point vector </summary>
					FLOAT4 = 3,
					

					/// <summary> 32 bit integer </summary>
					INT = 4,

					/// <summary> 32 bit 2d integer vector </summary>
					INT2 = 5,

					/// <summary> 32 bit 3d integer vector </summary>
					INT3 = 6,

					/// <summary> 32 bit 4d integer vector </summary>
					INT4 = 7,
					

					/// <summary> 32 bit unsigned integer </summary>
					UINT = 8,

					/// <summary> 32 bit 2d unsigned integer vector </summary>
					UINT2 = 9,

					/// <summary> 32 bit 3d unsigned integer vector </summary>
					UINT3 = 10,

					/// <summary> 32 bit 4d unsigned integer vector </summary>
					UINT4 = 11,

					
					/// <summary> 32 bit boolean value </summary>
					BOOL32 = 12,


					/// <summary> Single precision (32 bit) 2X2 floating point matrix </summary>
					MAT_2X2 = 13,

					/// <summary> Single precision (32 bit) 3X3 floating point matrix </summary>
					MAT_3X3 = 14,

					/// <summary> Single precision (32 bit) 4X4 floating point matrix </summary>
					MAT_4X4 = 15,

					/// <summary> Not an actual data type; denotes number of different types that can be accepted </summary>
					TYPE_COUNT = 16
				};

				/// <summary> Attribute type </summary>
				Type type;

				/// <summary> glsl location </summary>
				uint32_t location;

				/// <summary> Attribute offset within buffer element in bytes </summary>
				size_t offset;
			};

			/// <summary> Virtual destructor </summary>
			inline virtual ~VertexBuffer() {}

			/// <summary> Number of attributes exposed from each buffer element </summary>
			virtual size_t AttributeCount()const = 0;

			/// <summary>
			/// Exposed buffer elemnt attribute by index
			/// </summary>
			/// <param name="index"> Attribute index </param>
			/// <returns> Attribute description </returns>
			virtual AttributeInfo Attribute(size_t index)const = 0;

			/// <summary>
			/// Size of an individual element within the buffer;
			/// (Actual Buffer() may change over lifetime, this one has to stay constant)
			/// </summary>
			virtual size_t BufferElemSize()const = 0;

			/// <summary> Buffer, carrying the vertex/instance data </summary>
			virtual Reference<ArrayBuffer> Buffer() = 0;
		};

		/// <summary> Vertex/Instance buffer interface </summary>
		typedef VertexBuffer InstanceBuffer;
	}
}
