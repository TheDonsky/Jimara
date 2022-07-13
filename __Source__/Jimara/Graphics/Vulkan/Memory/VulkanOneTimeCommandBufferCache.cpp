#include "VulkanOneTimeCommandBufferCache.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanOneTimeCommandBufferCache::VulkanOneTimeCommandBufferCache(VulkanDevice* device) : m_device(device) {}

			VulkanOneTimeCommandBufferCache::~VulkanOneTimeCommandBufferCache() {}

			void VulkanOneTimeCommandBufferCache::Execute(Callback<VulkanPrimaryCommandBuffer*> recordCommands) {
				std::unique_lock<std::mutex> lock(m_lock);
				m_updateBuffers.push(m_device->SubmitOneTimeCommandBuffer(recordCommands));
				while (!m_updateBuffers.empty()) {
					const auto& info = m_updateBuffers.front();
					if (info.timeline->Count() < info.timelineValue) break;
					else m_updateBuffers.pop();
				}
			}
		}
	}
}
