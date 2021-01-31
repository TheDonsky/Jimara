#pragma once
#include "VulkanCommandPool.h"

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
				virtual VkCommandBuffer CommandBuffer()const = 0;

				/// <summary> 
				/// If there are resources that should stay alive during command buffer execution, but might otherwise go out of scope,
				/// user can record those in a kind of a set that will delay their destruction till buffer execution ends using this call.
				/// </summary>
				virtual void RecordBufferDependency(Object* object) = 0;

				/// <summary> Waits for the given semaphore before executing the command buffer </summary>
				virtual void WaitForSemaphore(VkSemaphore semaphore) = 0;

				/// <summary> Signals given semaphore when the command buffer gets executed </summary>
				virtual void SignalSemaphore(VkSemaphore semaphore) = 0;

				/// <summary> Command pool for handling additional command buffer creation and what not </summary>
				virtual VulkanCommandPool* CommandPool()const = 0;
			};
		}
	}
}
