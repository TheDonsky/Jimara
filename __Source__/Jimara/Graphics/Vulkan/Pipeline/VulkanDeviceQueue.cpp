#include "VulkanDeviceQueue.h"
#include "VulkanCommandBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanDeviceQueue::VulkanDeviceQueue(VkDeviceHandle* device, uint32_t queueFamilyId)
				: m_device(device), m_queueFamilyId(queueFamilyId)
				, m_queue([&]() -> VkQueue { VkQueue queue; vkGetDeviceQueue(*device, queueFamilyId, 0, &queue); return queue; }())
				, m_features([&]()-> FeatureBits {
				FeatureBits bits = 0;
				const VkQueueFlags flags = device->PhysicalDevice()->QueueFamilyProperties(queueFamilyId).queueFlags;
				if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0) bits |= static_cast<FeatureBits>(FeatureBit::GRAPHICS);
				if ((flags & VK_QUEUE_COMPUTE_BIT) != 0) bits |= static_cast<FeatureBits>(FeatureBit::COMPUTE);
				if ((flags & VK_QUEUE_TRANSFER_BIT) != 0) bits |= static_cast<FeatureBits>(FeatureBit::TRANSFER);
				return bits;
					}()) {}

			VulkanDeviceQueue::~VulkanDeviceQueue() {}

			VulkanDeviceQueue::operator VkQueue()const { return m_queue; }

			VkDeviceHandle* VulkanDeviceQueue::Device()const { return m_device; }

			uint32_t VulkanDeviceQueue::FamilyId()const { return m_queueFamilyId; }

			DeviceQueue::FeatureBits VulkanDeviceQueue::Features()const { return m_features; }

			Reference<CommandPool> VulkanDeviceQueue::CreateCommandPool() {
				return Object::Instantiate<VulkanCommandPool>(this, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			}

			void VulkanDeviceQueue::ExecuteCommandBuffer(PrimaryCommandBuffer* buffer) {
				VulkanPrimaryCommandBuffer* vulkanBuffer = dynamic_cast<VulkanPrimaryCommandBuffer*>(buffer);
				if (vulkanBuffer == nullptr) return;
				std::unique_lock<std::mutex> lock(m_submitionLock);
				vulkanBuffer->SumbitOnQueue(*this);
			}
		}
	}
}
