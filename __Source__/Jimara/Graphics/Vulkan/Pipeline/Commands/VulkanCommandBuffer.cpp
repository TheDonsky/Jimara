#include "VulkanCommandBuffer.h"
#include "../RenderPass/VulkanFrameBuffer.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer)
				: m_commandPool(commandPool), m_commandBuffer(buffer), m_unorderedAccessManager(this) {
				assert(RefCount() == 1u);
			}

			VulkanCommandBuffer::operator VkCommandBuffer()const { return m_commandBuffer; }

			VulkanCommandPool* VulkanCommandBuffer::CommandPool()const { return m_commandPool; }

			namespace {
				template<typename InfoType, typename WaitInfo>
				inline static void IncludeSemaphore(VkSemaphore semaphore, const WaitInfo& info, std::unordered_map<VkSemaphore, InfoType>& collection) {
					typename std::unordered_map<VkSemaphore, InfoType>::iterator it = collection.find(semaphore);
					if (it != collection.end()) {
						WaitInfo waitInfo = it->second;
						if (waitInfo.count < info.count)
							waitInfo.count = info.count;
						waitInfo.stageFlags |= info.stageFlags;
						it->second = waitInfo;
					}
					else collection[semaphore] = info;
				}
			}

			void VulkanCommandBuffer::WaitForSemaphore(VulkanSemaphore* semaphore, VkPipelineStageFlags waitStages) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, 0, waitStages), m_semaphoresToWait);
			}

			void VulkanCommandBuffer::WaitForSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count, VkPipelineStageFlags waitStages) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, count, waitStages), m_semaphoresToWait);
			}

			void VulkanCommandBuffer::SignalSemaphore(VulkanSemaphore* semaphore) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, 0, 0), m_semaphoresToSignal);
			}

			void VulkanCommandBuffer::SignalSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, count, 0), m_semaphoresToSignal);
			}

			void VulkanCommandBuffer::RecordBufferDependency(Object* dependency) { 
				m_bufferDependencies.push_back(dependency); 
			}

			VulkanUnorderedAccessStateManager& VulkanCommandBuffer::UnorderedAccess() {
				return m_unorderedAccessManager;
			}

			void VulkanCommandBuffer::GetSemaphoreDependencies(
				std::vector<VkSemaphore>& waitSemaphores, std::vector<uint64_t>& waitCounts, std::vector<VkPipelineStageFlags>& waitStages,
				std::vector<VkSemaphore>& signalSemaphores, std::vector<uint64_t>& signalCounts)const {
				for (std::unordered_map<VkSemaphore, WaitInfo>::const_iterator it = m_semaphoresToWait.begin(); it != m_semaphoresToWait.end(); ++it) {
					waitSemaphores.push_back(it->first);
					waitCounts.push_back(it->second.count);
					waitStages.push_back(it->second.stageFlags);
				}
				for (std::unordered_map<VkSemaphore, SemaphoreInfo>::const_iterator it = m_semaphoresToSignal.begin(); it != m_semaphoresToSignal.end(); ++it) {
					signalSemaphores.push_back(it->first);
					signalCounts.push_back(it->second.count);
				}
			}

			void VulkanCommandBuffer::Reset() {
				m_unorderedAccessManager.DisableUnorderedAccess();
				m_unorderedAccessManager.ClearBindingSetInfos();

				if (vkResetCommandBuffer(m_commandBuffer, 0) != VK_SUCCESS)
					m_commandPool->Queue()->Device()->Log()->Fatal("VulkanCommandBuffer - Can not reset command buffer!");
				
				m_semaphoresToWait.clear();
				m_semaphoresToSignal.clear();
				m_bufferDependencies.clear();
			}

			void VulkanCommandBuffer::EndRecording() {
				m_unorderedAccessManager.DisableUnorderedAccess();
				m_unorderedAccessManager.ClearBindingSetInfos();

				if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
					m_commandPool->Queue()->Device()->Log()->Fatal("VulkanCommandBuffer - Failed to end command buffer!");
			}

			void VulkanCommandBuffer::AddSemaphoreDependencies(const VulkanCommandBuffer* src, VulkanCommandBuffer* dst) {
				for (std::unordered_map<VkSemaphore, WaitInfo>::const_iterator it = src->m_semaphoresToWait.begin(); it != src->m_semaphoresToWait.end(); ++it) {
					VulkanTimelineSemaphore* semaphore = dynamic_cast<VulkanTimelineSemaphore*>(it->second.semaphore.operator->());
					if (semaphore != nullptr) dst->WaitForSemaphore(semaphore, it->second.count, it->second.stageFlags);
					else dst->WaitForSemaphore(dynamic_cast<VulkanSemaphore*>(it->second.semaphore.operator->()), it->second.stageFlags);
				}
				for (std::unordered_map<VkSemaphore, SemaphoreInfo>::const_iterator it = src->m_semaphoresToSignal.begin(); it != src->m_semaphoresToSignal.end(); ++it) {
					VulkanTimelineSemaphore* semaphore = dynamic_cast<VulkanTimelineSemaphore*>(it->second.semaphore.operator->());
					if (semaphore != nullptr) dst->SignalSemaphore(semaphore, it->second.count);
					else dst->SignalSemaphore(dynamic_cast<VulkanSemaphore*>(it->second.semaphore.operator->()));
				}
			}


			VulkanPrimaryCommandBuffer::VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer)
				: VulkanCommandBuffer(commandPool, buffer), m_fence(commandPool->Queue()->Device()), m_running(false) {}

			VulkanPrimaryCommandBuffer::~VulkanPrimaryCommandBuffer() {
				Wait();
			}
			
			void VulkanPrimaryCommandBuffer::BeginRecording() {
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0; // Optional
				beginInfo.pInheritanceInfo = nullptr; // Optional
				if (vkBeginCommandBuffer(*this, &beginInfo) != VK_SUCCESS)
					CommandPool()->Queue()->Device()->Log()->Fatal("VulkanPrimaryCommandBuffer - Failed to begin command buffer!");
			}

			void VulkanPrimaryCommandBuffer::Reset() {
				Wait();
				VulkanCommandBuffer::Reset();
			}

			void VulkanPrimaryCommandBuffer::Wait() {
				bool expected = true;
				bool running = m_running.compare_exchange_strong(expected, false);
				if (running && expected) m_fence.WaitAndReset();
			}

			void VulkanPrimaryCommandBuffer::EndRecording() {
				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_HOST_READ_BIT;
				}
				vkCmdPipelineBarrier(*this,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT 
					| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					VK_PIPELINE_STAGE_HOST_BIT,
					0, 1, &barrier, 0, nullptr, 0, nullptr);
				
				VulkanCommandBuffer::EndRecording();
			}

			void VulkanPrimaryCommandBuffer::ExecuteCommands(SecondaryCommandBuffer* commands) {
				VulkanSecondaryCommandBuffer* vulkanBuffer = dynamic_cast<VulkanSecondaryCommandBuffer*>(commands);
				if (vulkanBuffer == nullptr) {
					CommandPool()->Queue()->Device()->Log()->Fatal("VulkanPrimaryCommandBuffer::ExecuteCommands - Invalid secondary command buffer provided!");
					return;
				}
				VkCommandBuffer buffer = *vulkanBuffer;
				vkCmdExecuteCommands(*this, 1, &buffer);
				RecordBufferDependency(vulkanBuffer);
				AddSemaphoreDependencies(vulkanBuffer, this);
			}

			void VulkanPrimaryCommandBuffer::SumbitOnQueue(VulkanDeviceQueue* queue) {
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
					GetSemaphoreDependencies(waitSemaphores, waitValues, waitStages, signalSemaphores, signalValues);
				}

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
				VkCommandBuffer apiHandle = *this;
				{
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.pNext = &timelineInfo;

					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
					submitInfo.pWaitSemaphores = waitSemaphores.data();
					submitInfo.pWaitDstStageMask = waitStages.data();

					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &apiHandle;

					submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
					submitInfo.pSignalSemaphores = signalSemaphores.data();
				}

				Wait();
				if (queue->Submit(submitInfo, &m_fence) != VK_SUCCESS)
					CommandPool()->Queue()->Device()->Log()->Fatal("VulkanPrimaryCommandBuffer - Failed to submit command buffer!");
				else m_running = true;
			}

			VulkanSecondaryCommandBuffer::VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer)
				: VulkanCommandBuffer(commandPool, buffer) {}

			void VulkanSecondaryCommandBuffer::BeginRecording(RenderPass* activeRenderPass, FrameBuffer* targetFrameBuffer) {
				VulkanRenderPass* vulkanPass = dynamic_cast<VulkanRenderPass*>(activeRenderPass);

				VkCommandBufferInheritanceInfo inheritance = {};
				inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritance.renderPass = (vulkanPass == nullptr) ? VK_NULL_HANDLE : ((VkRenderPass)(*vulkanPass));
				inheritance.subpass = 0;

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = (vulkanPass == nullptr) ? 0 : VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
				beginInfo.pInheritanceInfo = &inheritance;

				if (vkBeginCommandBuffer(*this, &beginInfo) != VK_SUCCESS)
					CommandPool()->Queue()->Device()->Log()->Fatal("VulkanSecondaryCommandBuffer - Failed to begin command buffer!");
				else if (targetFrameBuffer != nullptr) {
					// Set viewport & scisors
					const Size2 size = targetFrameBuffer->Resolution();
					VkRect2D scisior = {};
					scisior.offset = { 0, 0 };
					scisior.extent = { size.x, size.y };

					vkCmdSetScissor(*this, 0, 1, &scisior);
					
					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = (float)scisior.extent.height;
					viewport.width = (float)scisior.extent.width;
					viewport.height = -viewport.y;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					vkCmdSetViewport(*this, 0, 1, &viewport);
				}

				RecordBufferDependency(vulkanPass);
			}
		}
	}
}
