#pragma once
#include "../Pipeline/VulkanCommandRecorder.h"
#include "../../../Core/Function.h"
#include <queue>
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDynamicDataUpdater {
			public:
				VulkanDynamicDataUpdater(VkDeviceHandle* device);

				virtual ~VulkanDynamicDataUpdater();

				void WaitForTimeline(VulkanCommandBuffer* commandBuffer);

				void Update(VulkanCommandBuffer* commandBuffer, Callback<VulkanCommandBuffer*> dataUpdateFn);

			private:
				Reference<VulkanTimelineSemaphore> m_timeline;

				std::atomic<uint64_t> m_revision;

				std::atomic<uint64_t> m_lastKnownRevision;

				std::queue<std::pair<Reference<VulkanPrimaryCommandBuffer>, uint64_t>> m_updateBuffers;

				std::mutex m_lock;
			};
		}
	}
}
