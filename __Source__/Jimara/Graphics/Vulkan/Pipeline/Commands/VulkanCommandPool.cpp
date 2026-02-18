#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 4250)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCommandPool::VulkanCommandPool(VulkanDeviceQueue* queue, VkCommandPoolCreateFlags createFlags)
				: m_queue(queue), m_createFlags(createFlags)
				, m_commandPool([&]()->VkCommandPool {
				VkCommandPoolCreateInfo poolInfo = {};
				{
					poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					poolInfo.queueFamilyIndex = queue->FamilyId();
					poolInfo.flags = m_createFlags;
				}
				VkCommandPool pool = VK_NULL_HANDLE;
				if (vkCreateCommandPool(*queue->Device(), &poolInfo, nullptr, &pool) != VK_SUCCESS) {
					pool = VK_NULL_HANDLE;
					queue->Device()->Log()->Fatal("VulkanCommandPool - Failed to create command pool!");
				}
				return pool;
					}()) {}

			VulkanCommandPool::~VulkanCommandPool() {
				FreeOutOfScopeCommandBuffers();
				if (m_commandPool != VK_NULL_HANDLE)
					vkDestroyCommandPool(*m_queue->Device(), m_commandPool, nullptr);
			}

			VulkanDeviceQueue* VulkanCommandPool::Queue()const { return m_queue; }

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
					if (vkAllocateCommandBuffers(*commandPool->Queue()->Device(), &allocInfo, buffers) != VK_SUCCESS)
						commandPool->Queue()->Device()->Log()->Fatal("VulkanCommandPool - Failed to allocate command buffers!");
				}
			}

			std::vector<VkCommandBuffer> VulkanCommandPool::CreateCommandBuffers(size_t count, VkCommandBufferLevel level)const {
				FreeOutOfScopeCommandBuffers();
				std::vector<VkCommandBuffer> commandBuffers(count);
				AllocateCommandBuffers(this, level, count, commandBuffers.data());
				return commandBuffers;
			}

			VkCommandBuffer VulkanCommandPool::CreateCommandBuffer(VkCommandBufferLevel level)const {
				FreeOutOfScopeCommandBuffers();
				VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
				AllocateCommandBuffers(this, level, 1, &commandBuffer);
				return commandBuffer;
			}

			void VulkanCommandPool::DestroyCommandBuffers(const VkCommandBuffer* buffers, size_t count)const {
				if (buffers != nullptr && count > 0) {
					std::unique_lock<std::mutex> lock(m_outOfScopeLock);
					for (size_t i = 0; i < count; i++) m_outOfScopeBuffers.push_back(buffers[i]);
				}
			}

			void VulkanCommandPool::DestroyCommandBuffers(std::vector<VkCommandBuffer>& buffers)const {
				DestroyCommandBuffers(buffers.data(), buffers.size());
				buffers.clear();
			}

			void VulkanCommandPool::DestroyCommandBuffer(VkCommandBuffer buffer)const {
				DestroyCommandBuffers(&buffer, 1);
			}

			void VulkanCommandPool::SubmitSingleTimeCommandBuffer(const Callback<VkCommandBuffer>& recordCallback) {
				VkCommandBuffer commandBuffer = CreateCommandBuffer();
				{
					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					vkBeginCommandBuffer(commandBuffer, &beginInfo);
				}
				recordCallback(commandBuffer);
				vkEndCommandBuffer(commandBuffer);
				{
					VkSubmitInfo submitInfo = {};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &commandBuffer;
					Queue()->Submit(submitInfo, VK_NULL_HANDLE);
				}
				Queue()->WaitIdle();
				DestroyCommandBuffer(commandBuffer);
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

					inline virtual ~BatchCommandBufferInstance() {
						PrimaryCommandBuffer* primary = dynamic_cast<VulkanPrimaryCommandBuffer*>(this);
						if (primary != nullptr) primary->Wait();
					}
				};

				typedef BatchCommandBufferInstance<VulkanPrimaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> BatchPrimaryCommandBufferInstance;

				typedef BatchCommandBufferInstance<VulkanSecondaryCommandBuffer, VulkanCommandPool*, VkCommandBuffer> BatchSecondaryCommandBufferInstance;


				template<typename Parent, typename... ParentConstructorArgs>
				class SingleCommandBufferInstance : public Parent {
				public:
					inline SingleCommandBufferInstance(ParentConstructorArgs... parentArgs)
						: Parent(parentArgs...) {}

					inline virtual ~SingleCommandBufferInstance() {
						PrimaryCommandBuffer* primary = dynamic_cast<VulkanPrimaryCommandBuffer*>(this);
						if (primary != nullptr) primary->Wait();
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


			Reference<SecondaryCommandBuffer> VulkanCommandPool::CreateSecondaryCommandBuffer() {
				return Object::Instantiate<SingleSecondaryCommandBufferInstance>(this, CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
			}

			std::vector<Reference<SecondaryCommandBuffer>> VulkanCommandPool::CreateSecondaryCommandBuffers(size_t count) {
				Reference<VkCommandBufferBatch> batch = Object::Instantiate<VkCommandBufferBatch>(this, count, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
				std::vector<Reference<SecondaryCommandBuffer>> buffers(count);
				for (size_t i = 0; i < count; i++) buffers[i] = Object::Instantiate<BatchSecondaryCommandBufferInstance>(batch, this, (*batch)[i]);
				return buffers;
			}

			void VulkanCommandPool::FreeOutOfScopeCommandBuffers()const {
				std::unique_lock<std::mutex> lock(m_outOfScopeLock);
				if (m_outOfScopeBuffers.size() > 0) {
					vkFreeCommandBuffers(*m_queue->Device(), m_commandPool, static_cast<uint32_t>(m_outOfScopeBuffers.size()), m_outOfScopeBuffers.data());
					m_outOfScopeBuffers.clear();
				}
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 4250)
