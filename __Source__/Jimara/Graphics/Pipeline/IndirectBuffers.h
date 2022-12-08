#pragma once
#include "../Memory/Buffers.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Structure specifying a indexed indirect drawing command
		/// </summary>
		struct JIMARA_API DrawIndirectCommand {
			/// <summary> The number of vertices to draw </summary>
			alignas(4) uint32_t indexCount = 0u;

			/// <summary> The number of instances to draw </summary>
			alignas(4) uint32_t instanceCount = 0u;

			/// <summary> The base index within the index buffer </summary>
			alignas(4) uint32_t firstIndex = 0u;

			/// <summary> The value added to the vertex index before indexing into the vertex buffer </summary>
			alignas(4) int32_t vertexOffset = 0;

			/// <summary> The instance ID of the first instance to draw </summary>
			alignas(4) uint32_t firstInstance = 0u;
		};

		/// <summary>
		/// Indirect draw buffer;
		/// <para/> Just an ArrayBuffer of DrawIndirectCommand-s will not suffice, since for different backend APIs the command structure may be different.
		/// </summary>
		class JIMARA_API IndirectDrawBuffer : public virtual ArrayBuffer {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~IndirectDrawBuffer() {}

			/// <summary>
			/// Maps indirect draw buffer memory to CPU
			/// <para/> Notes:
			///		<para/> 0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		<para/> 1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
			///		 the actual content of the buffer will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			inline IndirectDrawBuffer* MapCommands() { return reinterpret_cast<IndirectDrawBuffer*>(Map()); }
		};

		/// <summary> Type definition of an IndirectDrawBuffer reference </summary>
		typedef Reference<IndirectDrawBuffer> IndirectDrawBufferReference;
	}
}
