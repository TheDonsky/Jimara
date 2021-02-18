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


		class CommandBuffer : public virtual Object {
		public:
			virtual void Reset() = 0;

			virtual void BeginRecording() = 0;

			virtual void EndRecording() = 0;
		};

		class PrimaryCommandBuffer : public virtual CommandBuffer {
		public:
			virtual void Wait() = 0;
		};

		class SecondaryCommandBuffer : public virtual CommandBuffer {
		public:

		};
	}
}
