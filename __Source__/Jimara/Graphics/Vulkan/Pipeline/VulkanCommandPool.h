#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanCommandPool;
		}
	}
}
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Wrapper on top of VkCommandPool
			/// </summary>
			class VulkanCommandPool : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Logical device </param>
				/// <param name="queueFamilyId"> Queue family id </param>
				/// <param name="createFlags"> Command pool create flags </param>
				VulkanCommandPool(VkDeviceHandle* device, uint32_t queueFamilyId, VkCommandPoolCreateFlags createFlags);

				/// <summary>
				/// Constructor
				/// Note: queueFamilyId defaults to main graphics queue
				/// </summary>
				/// <param name="device"> Logical device </param>
				/// <param name="createFlags"> Command pool create flags </param>
				VulkanCommandPool(VkDeviceHandle* device, VkCommandPoolCreateFlags createFlags);

				/// <summary>
				/// Constructor
				/// Note: queueFamilyId defaults to main graphics queue; createFlags defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
				/// </summary>
				/// <param name="device"> Logical device </param>
				VulkanCommandPool(VkDeviceHandle* device);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanCommandPool();

				/// <summary> "Owner" device </summary>
				VkDeviceHandle* Device()const;

				/// <summary> Target queue family id </summary>
				uint32_t QueueFamilyId()const;

				/// <summary> Target queue </summary>
				VkQueue Queue()const;

				/// <summary> Command pool create flags used during creation </summary>
				VkCommandPoolCreateFlags CreateFlags()const;

				/// <summary> Type cast to underlying API object </summary>
				operator VkCommandPool()const;

				/// <summary>
				/// Creates command buffers
				/// </summary>
				/// <param name="count"> Number of buffers to create </param>
				/// <param name="level"> Created buffer level </param>
				/// <returns> Vector of newly created buffers </returns>
				std::vector<VkCommandBuffer> CreateCommandBuffers(size_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)const;

				/// <summary>
				/// Creates a single command buffer
				/// </summary>
				/// <param name="level"> Created buffer level </param>
				/// <returns> New command buffer </returns>
				VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)const;

				/// <summary>
				/// Destroys command buffers
				/// </summary>
				/// <param name="buffers"> Buffers to destroy (should be created "togather" with single CreateCommandBuffers() call) </param>
				/// <param name="count"> Number of buffers within buffer array </param>
				void DestroyCommandBuffers(VkCommandBuffer* buffers, size_t count)const;

				/// <summary>
				/// Destroys a single command buffer
				/// </summary>
				/// <param name="buffers"> Command buffers to destroy (should be created "togather" with single CreateCommandBuffers() call) </param>
				void DestroyCommandBuffers(std::vector<VkCommandBuffer>& buffers)const;

				/// <summary>
				/// Destroys a single command buffer
				/// </summary>
				/// <param name="buffer"> Command buffer to destroy </param>
				void DestroyCommandBuffer(VkCommandBuffer buffer)const;

				/// <summary>
				/// Creates and runs a single-time command buffer (introduces sync point, so use this one with an amount of caution)
				/// </summary>
				/// <typeparam name="FunctionType"> Some function type that gets a VkCommandBuffer as parameter, records some commands to it and returns </typeparam>
				/// <param name="recordCallback"> Some function that gets a VkCommandBuffer as parameter, records some commands to it and returns </param>
				template<typename FunctionType>
				inline void SubmitSingleTimeCommandBuffer(FunctionType recordCallback) {
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
						vkQueueSubmit(Queue(), 1, &submitInfo, VK_NULL_HANDLE);
					}
					vkQueueWaitIdle(Queue());
					DestroyCommandBuffer(commandBuffer);
				}



			private:
				// "Owener" device
				Reference<VkDeviceHandle> m_device;
				
				// Target queue family id
				uint32_t m_queueFamilyId;

				// Pool create flags
				VkCommandPoolCreateFlags m_createFlags;

				// Underlying command pool
				VkCommandPool m_commandPool;

				// Target queue
				VkQueue m_queue;
			};
		}
	}
}
