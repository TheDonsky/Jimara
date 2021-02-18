#pragma once
namespace Jimara {
	namespace Graphics {
		class CommandPool;
		class CommandBuffer;
		class PrimaryCommandBuffer;
		class SecondaryCommandBuffer;
	}
}
#include "../../Core/Object.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Command pool for creating command buffers;
		/// </summary>
		class CommandPool : public virtual Object {
		public:
			/// <summary> Creates a primary command buffer </summary>
			virtual Reference<PrimaryCommandBuffer> CreatePrimaryCommandBuffer() = 0;

			/// <summary>
			/// Creates a bounch of primary command buffers
			/// </summary>
			/// <param name="count"> Number of command buffers to instantiate </param>
			/// <returns> List of command buffers </returns>
			virtual std::vector<Reference<PrimaryCommandBuffer>> CreatePrimaryCommandBuffers(size_t count) = 0;
		};

		/// <summary>
		/// Command buffer for graphics command recording
		/// </summary>
		class CommandBuffer : public virtual Object {
		public:
			/// <summary> Resets command buffer and all of it's internal state previously recorded </summary>
			virtual void Reset() = 0;

			/// <summary> Starts recording the command buffer (does NOT auto-invoke Reset()) </summary>
			virtual void BeginRecording() = 0;

			/// <summary> Ends recording the command buffer </summary>
			virtual void EndRecording() = 0;
		};

		/// <summary>
		/// Command buffer, that can directly be executed on a graphics queue
		/// </summary>
		class PrimaryCommandBuffer : public virtual CommandBuffer {
		public:
			/// <summary> If the command buffer has been previously submitted, this call will wait on execution wo finish </summary>
			virtual void Wait() = 0;
		};

		/// <summary>
		/// A secondary command buffer that can be recorded separately from primary command buffer and later executed as a part of it
		/// </summary>
		class SecondaryCommandBuffer : public virtual CommandBuffer {
		public:
		};
	}
}
