#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCommandPool::VulkanCommandPool(VkDeviceHandle* device, uint32_t queueFamilyId, VkCommandPoolCreateFlags createFlags)
				: m_device(device), m_queueFamilyId(queueFamilyId), m_createFlags(createFlags), m_commandPool(VK_NULL_HANDLE), m_queue(VK_NULL_HANDLE) {
				VkCommandPoolCreateInfo poolInfo = {};
				{
					poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					poolInfo.queueFamilyIndex = m_queueFamilyId;
					poolInfo.flags = m_createFlags;
				}
				if (vkCreateCommandPool(*device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanCommandPool - Failed to create command pool!");

				vkGetDeviceQueue(*m_device, m_queueFamilyId, 0, &m_queue);
			}

			VulkanCommandPool::VulkanCommandPool(VkDeviceHandle* device, VkCommandPoolCreateFlags createFlags)
				: VulkanCommandPool(device, device->PhysicalDevice()->GraphicsQueueId().value(), createFlags) { }

			VulkanCommandPool::VulkanCommandPool(VkDeviceHandle* device)
				: VulkanCommandPool(device, device->PhysicalDevice()->GraphicsQueueId().value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) { }

			VulkanCommandPool::~VulkanCommandPool() {
				if (m_commandPool != VK_NULL_HANDLE) {
					vkDestroyCommandPool(*m_device, m_commandPool, nullptr);
					m_commandPool = VK_NULL_HANDLE;
				}
			}

			VkDeviceHandle* VulkanCommandPool::Device()const { return m_device; }

			uint32_t VulkanCommandPool::QueueFamilyId()const { return m_queueFamilyId; }

			VkQueue VulkanCommandPool::Queue()const { return m_queue; }

			VkCommandPoolCreateFlags VulkanCommandPool::CreateFlags()const { return m_createFlags; }

			VulkanCommandPool::operator VkCommandPool()const { return m_commandPool; }

			namespace {
				inline static void AllocateCommandBuffers(const VulkanCommandPool* commandPool, VkCommandBufferLevel level, size_t count, VkCommandBuffer* buffers) {
					VkCommandBufferAllocateInfo allocInfo = {};
					{
						allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
						allocInfo.commandPool = *commandPool;
						allocInfo.level = level;
						allocInfo.commandBufferCount = static_cast<uint32_t>(count);
					}
					if (vkAllocateCommandBuffers(*commandPool->Device(), &allocInfo, buffers) != VK_SUCCESS)
						commandPool->Device()->Log()->Fatal("VulkanCommandPool - Failed to allocate command buffers!");
				}
			}

			std::vector<VkCommandBuffer> VulkanCommandPool::CreateCommandBuffers(size_t count, VkCommandBufferLevel level)const {
				std::vector<VkCommandBuffer> commandBuffers(count);
				AllocateCommandBuffers(this, level, count, commandBuffers.data());
				return commandBuffers;
			}

			VkCommandBuffer VulkanCommandPool::CreateCommandBuffer(VkCommandBufferLevel level)const {
				VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
				AllocateCommandBuffers(this, level, 1, &commandBuffer);
				return commandBuffer;
			}

			void VulkanCommandPool::DestroyCommandBuffers(const VkCommandBuffer* buffers, size_t count)const {
				if (buffers != nullptr && count > 0)
					vkFreeCommandBuffers(*m_device, m_commandPool, static_cast<uint32_t>(count), buffers);
			}

			void VulkanCommandPool::DestroyCommandBuffers(std::vector<VkCommandBuffer>& buffers)const {
				DestroyCommandBuffers(buffers.data(), buffers.size());
				buffers.clear();
			}

			void VulkanCommandPool::DestroyCommandBuffer(VkCommandBuffer buffer)const {
				DestroyCommandBuffers(&buffer, 1);
			}

			namespace {
				class VkCommandBufferBatch : public virtual Object {
				private:
					const Reference<VulkanCommandPool> m_pool;
					const std::vector<VkCommandBuffer> m_buffers;

				public:
					inline VkCommandBufferBatch(VulkanCommandPool* commandPool, size_t count, VkCommandBufferLevel level)
						: m_pool(commandPool), m_buffers(commandPool->CreateCommandBuffers(count, level)) {}

					inline virtual ~VkCommandBufferBatch() { m_pool->DestroyCommandBuffers(m_buffers.data(), m_buffers.size()); }

					inline VkCommandBuffer operator[](size_t index) { return m_buffers[index]; }
				};

				template<typename Parent, typename... ParentConstructorArgs>
				class BatchCommandBufferInstance : public Parent {
				private:
					const Reference<VkCommandBufferBatch> m_batch;

				public:
					inline BatchCommandBufferInstance(VkCommandBufferBatch* batch, ParentConstructorArgs... parentArgs)
						: Parent(parentArgs...), m_batch(batch) {}
				};

				typedef BatchCommandBufferInstance<VulkanPrimaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> BatchPrimaryCommandBufferInstance;

				typedef BatchCommandBufferInstance<VulkanSecondaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> BatchSecondaryCommandBufferInstance;


				template<typename Parent, typename... ParentConstructorArgs>
				class SingleCommandBufferInstance : public Parent {
				public:
					inline SingleCommandBufferInstance(ParentConstructorArgs... parentArgs)
						: Parent(parentArgs...) {}

					inline virtual ~SingleCommandBufferInstance() {
						VkCommandBuffer buffer = (*this);
						if (buffer != VK_NULL_HANDLE)
							this->CommandPool()->DestroyCommandBuffer(buffer);
					}
				};

				typedef SingleCommandBufferInstance<VulkanPrimaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> SinglePrimaryCommandBufferInstance;

				typedef SingleCommandBufferInstance<VulkanSecondaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> SingleSecondaryCommandBufferInstance;
			}

			Reference<PrimaryCommandBuffer> VulkanCommandPool::CreatePrimaryCommandBuffer() {
				return Object::Instantiate<SinglePrimaryCommandBufferInstance>(this, CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
			}

			std::vector<Reference<PrimaryCommandBuffer>> VulkanCommandPool::CreatePrimaryCommandBuffers(size_t count) {
				Reference<VkCommandBufferBatch> batch = Object::Instantiate<VkCommandBufferBatch>(this, count, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
				std::vector<Reference<PrimaryCommandBuffer>> buffers(count);
				for (size_t i = 0; i < count; i++) buffers[i] = Object::Instantiate<BatchPrimaryCommandBufferInstance>(batch, this, (*batch)[i]);
				return buffers;
			}
		}
	}
}
#pragma warning(default: 26812)
