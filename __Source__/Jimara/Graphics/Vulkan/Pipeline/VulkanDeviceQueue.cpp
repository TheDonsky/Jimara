#include "VulkanDeviceQueue.h"
#include "VulkanCommandBuffer.h"

#define VULKAN_DEVICE_QUEUE_SUBMITION_BUFFER_SIZE 1024u
#define VULKAN_TIMELINE_SEMAPHORE_MAX_VAL (~((uint64_t)(0u)))

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
					}())
				, m_submitionPointer(0) {
				for (size_t i = 0; i < VULKAN_DEVICE_QUEUE_SUBMITION_BUFFER_SIZE; i++) {
					SubmitionInfo info = {};
					info.semaphore = Object::Instantiate<VulkanTimelineSemaphore>(m_device);
					m_submitionBuffer.push_back(info);
				}
			}

			VulkanDeviceQueue::~VulkanDeviceQueue() {}

			VulkanDeviceQueue::operator VkQueue()const { return m_queue; }

			VkDeviceHandle* VulkanDeviceQueue::Device()const { return m_device; }

			uint32_t VulkanDeviceQueue::FamilyId()const { return m_queueFamilyId; }

			DeviceQueue::FeatureBits VulkanDeviceQueue::Features()const { return m_features; }

			Reference<CommandPool> VulkanDeviceQueue::CreateCommandPool() {
				return Object::Instantiate<VulkanCommandPool>(m_device, m_queueFamilyId);
			}

			void VulkanDeviceQueue::ExecuteCommandBuffer(PrimaryCommandBuffer* buffer) {
				// If command buffer is not a valid vulkan command buffer, it makes no sence to submit this one..
				VulkanPrimaryCommandBuffer* vulkanBuffer = dynamic_cast<VulkanPrimaryCommandBuffer*>(buffer);
				if (vulkanBuffer == nullptr) return;

				// SubmitInfo needs the list of semaphores to wait for and their corresponding values:
				static thread_local std::vector<VkSemaphore> waitSemaphores;
				static thread_local std::vector<uint64_t> waitValues;
				static thread_local std::vector<VkPipelineStageFlags> waitStages;

				// SubmitInfo needs the list of semaphores to signal and their corresponding values:
				static thread_local std::vector<VkSemaphore> signalSemaphores;
				static thread_local std::vector<uint64_t> signalValues;

				// Clear semaphore lists and refill them from command buffer:
				{
					waitSemaphores.clear();
					waitValues.clear();
					waitStages.clear();
					signalSemaphores.clear();
					signalValues.clear();
					vulkanBuffer->GetSemaphoreDependencies(waitSemaphores, waitValues, waitStages, signalSemaphores, signalValues);
				}

				// From now on, we deal with the ring buffer and submition, so synchronisation is required:
				std::unique_lock<std::mutex> lock(m_submitionLock);
				
				// Here we get submition cache handle:
				size_t bufferId = m_submitionPointer.fetch_add(1);
				m_submitionPointer = m_submitionPointer % m_submitionBuffer.size();
				SubmitionInfo& info = m_submitionBuffer[bufferId];

				// If there's a command buffer currently submitted, we should wait for it's completion to avoid incorrect behaviour:
				if (info.commandBuffer != nullptr && info.commandBuffer != vulkanBuffer)
					info.semaphore->Wait(info.counter);
				
				// Update cache buffer:
				info.commandBuffer = vulkanBuffer;

				// Make sure to handle semaphore overflow gracefully:
				if (info.counter == VULKAN_TIMELINE_SEMAPHORE_MAX_VAL) {
					info.semaphore->Wait(VULKAN_TIMELINE_SEMAPHORE_MAX_VAL);
					info.semaphore->Set(0);
					info.counter = 1;
				}
				else info.counter++;
				
				// We need to signal cache semaphore, regardless of the other requirenments:
				signalSemaphores.push_back(*info.semaphore);
				signalValues.push_back(info.counter);



				// Submition:
				VkTimelineSemaphoreSubmitInfo timelineInfo = {};
				{
					timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
					timelineInfo.pNext = nullptr;

					timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
					timelineInfo.pWaitSemaphoreValues = waitValues.data();

					timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
					timelineInfo.pSignalSemaphoreValues = signalValues.data();
				}

				VkSubmitInfo submitInfo = {};
				{
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.pNext = &timelineInfo;

					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
					submitInfo.pWaitSemaphores = waitSemaphores.data();
					submitInfo.pWaitDstStageMask = waitStages.data();
					
					submitInfo.commandBufferCount = 1;
					VkCommandBuffer apiHandle = *vulkanBuffer;
					submitInfo.pCommandBuffers = &apiHandle;

					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
					submitInfo.pSignalSemaphores = signalSemaphores.data();
				}

				if (vkQueueSubmit(*this, 1, &submitInfo, nullptr) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanDeviceQueue - Failed to submit command buffer!");
			}
		}
	}
}
