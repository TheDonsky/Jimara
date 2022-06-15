#pragma once
namespace Jimara {
	namespace Graphics {
		class CommandPool;
		class CommandBuffer;
		class PrimaryCommandBuffer;
		class SecondaryCommandBuffer;

		class RenderPass;
		class FrameBuffer;
	}
}
#include "../../Core/Object.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Command pool for creating command buffers;
		/// </summary>
		class JIMARA_API CommandPool : public virtual Object {
		public:
			/// <summary> Creates a primary command buffer </summary>
			virtual Reference<PrimaryCommandBuffer> CreatePrimaryCommandBuffer() = 0;

			/// <summary>
			/// Creates a bounch of primary command buffers
			/// </summary>
			/// <param name="count"> Number of command buffers to instantiate </param>
			/// <returns> List of command buffers </returns>
			virtual std::vector<Reference<PrimaryCommandBuffer>> CreatePrimaryCommandBuffers(size_t count) = 0;

			/// <summary> Creates a secondary command buffer </summary>
			virtual Reference<SecondaryCommandBuffer> CreateSecondaryCommandBuffer() = 0;

			/// <summary>
			/// Creates a bounch of secondary command buffers
			/// </summary>
			/// <param name="count"> Number of command buffers to instantiate </param>
			/// <returns> List of command buffers </returns>
			virtual std::vector<Reference<SecondaryCommandBuffer>> CreateSecondaryCommandBuffers(size_t count) = 0;
		};

		/// <summary>
		/// Command buffer for graphics command recording
		/// </summary>
		class JIMARA_API CommandBuffer : public virtual Object {
		public:
			/// <summary> Starts recording the command buffer (does NOT auto-invoke Reset()) </summary>
			virtual void BeginRecording() = 0;

			/// <summary> Resets command buffer and all of it's internal state previously recorded </summary>
			virtual void Reset() = 0;

			/// <summary> Ends recording the command buffer </summary>
			virtual void EndRecording() = 0;
		};

		/// <summary>
		/// Command buffer, that can directly be executed on a graphics queue
		/// </summary>
		class JIMARA_API PrimaryCommandBuffer : public virtual CommandBuffer {
		public:
			/// <summary> If the command buffer has been previously submitted, this call will wait on execution wo finish </summary>
			virtual void Wait() = 0;

			/// <summary>
			/// Executes commands from a secondary command buffer
			/// </summary>
			/// <param name="commands"> Command buffer to execute </param>
			virtual void ExecuteCommands(SecondaryCommandBuffer* commands) = 0;
		};

		/// <summary>
		/// A secondary command buffer that can be recorded separately from primary command buffer and later executed as a part of it
		/// </summary>
		class JIMARA_API SecondaryCommandBuffer : public virtual CommandBuffer {
		public:
			/// <summary>
			/// Begins command buffer recording
			/// </summary>
			/// <param name="activeRenderPass"> Render pass, that will be active during the command buffer execution (can be nullptr, if there's no active pass) </param>
			/// <param name="targetFrameBuffer"> If the command buffer is meant to be used as a part of a render pass, this will be our target frame buffer </param>
			virtual void BeginRecording(RenderPass* activeRenderPass, FrameBuffer* targetFrameBuffer) = 0;

			/// <summary> Starts recording the command buffer that's meant to be executed outside a render pass </summary>
			inline virtual void BeginRecording() override { BeginRecording(nullptr, nullptr); }
		};
	}
}
