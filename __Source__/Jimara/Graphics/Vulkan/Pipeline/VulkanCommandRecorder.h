#pragma once
#include "VulkanDeviceQueue.h"
#include "VulkanCommandBuffer.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Command recorder.
			/// Note: When the graphics engine performs an update, 
			///		it requests all the underlying renderers to record their in-flight buffer specific commands 
			///		to a pre-constructed command buffer from the engine.
			/// </summary>
			class VulkanCommandRecorder {
			public:
				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanCommandRecorder() {}

				/// <summary> Target image index </summary>
				virtual size_t CommandBufferIndex()const = 0;

				/// <summary> Command buffer to record to </summary>
				virtual VulkanCommandBuffer* CommandBuffer()const = 0;
			};
		}
	}
}
