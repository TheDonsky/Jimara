#pragma once
namespace Jimara { namespace Graphics { class DeviceQueue; } }
#include "CommandBuffer.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Represents a command queue on graphics device
		/// </summary>
		class JIMARA_API DeviceQueue : public virtual Object {
		public:
			/// <summary>
			/// Features, supported on the queue
			/// </summary>
			enum class FeatureBit : uint8_t {
				/// <summary> All queus support doing nothing </summary>
				NOTHING = 0,

				/// <summary> Graphics command submition </summary>
				GRAPHICS = 1,

				/// <summary> Compute command submition (main graphics queue is expected to have this as well) </summary>
				COMPUTE = (1 << 1),

				/// <summary> Transfer command submition (anything with graphics should have it in theory) </summary>
				TRANSFER = (1 << 2)
			};

			/// <summary> Bitmask of FeatureBit flags </summary>
			typedef uint8_t FeatureBits;

			/// <summary> Features, supported by the queue </summary>
			virtual FeatureBits Features()const = 0;

			/// <summary> Creates a new instance of a command pool </summary>
			virtual Reference<CommandPool> CreateCommandPool() = 0;

			/// <summary> Executes command buffer on the queue </summary>
			/// <param name="buffer"> Command buffer </param>
			virtual void ExecuteCommandBuffer(PrimaryCommandBuffer* buffer) = 0;
		};
	}
}
