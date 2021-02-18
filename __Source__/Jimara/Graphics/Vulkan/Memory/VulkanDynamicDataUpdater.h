#pragma once
#include "../Pipeline/VulkanCommandRecorder.h"
#include "../../../Core/Function.h"
#include <queue>
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Helper for some dynamic storage types that need to execute a bounch of commands before being available to the main render logic
			/// </summary>
			class VulkanDynamicDataUpdater {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Device </param>
				VulkanDynamicDataUpdater(VkDeviceHandle* device);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDynamicDataUpdater();

				/// <summary>
				/// In case the last submitted update commands are not yet complete, this function will add the correct execution dependency to givaen command buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to add dependency to </param>
				void WaitForTimeline(VulkanCommandBuffer* commandBuffer);

				/// <summary>
				/// Creates and runs a command buffer, executing some updates and moving the object's personal timeline
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record dependencies to </param>
				/// <param name="dataUpdateFn"> Callback for updating arbitrary data </param>
				void Update(VulkanCommandBuffer* commandBuffer, Callback<VulkanCommandBuffer*> dataUpdateFn);


			private:
				// Personal timeline
				Reference<VulkanTimelineSemaphore> m_timeline;

				// Last submitted revision (timeline counter)
				std::atomic<uint64_t> m_revision;

				// Last synchronized revision (timeline counter)
				std::atomic<uint64_t> m_lastKnownRevision;

				// Submitted command buffers (We need these to avoid CPU-GPU synchronisations mid render)
				std::queue<std::pair<Reference<VulkanPrimaryCommandBuffer>, uint64_t>> m_updateBuffers;

				// Lock for the timeline and buffer queue
				std::mutex m_lock;
			};
		}
	}
}
