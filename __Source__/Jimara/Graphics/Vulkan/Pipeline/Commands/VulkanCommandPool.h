#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanCommandPool;
		}
	}
}
#include "VulkanDeviceQueue.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Wrapper on top of VkCommandPool
			/// </summary>
			class JIMARA_API VulkanCommandPool : public virtual CommandPool {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="queue"> Command queue </param>
				/// <param name="createFlags"> Command pool create flags </param>
				VulkanCommandPool(VulkanDeviceQueue* queue, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanCommandPool();

				/// <summary> Target queue </summary>
				VulkanDeviceQueue* Queue()const;

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
				void DestroyCommandBuffers(const VkCommandBuffer* buffers, size_t count)const;

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
						Queue()->Submit(submitInfo, VK_NULL_HANDLE);
					}
					Queue()->WaitIdle();
					DestroyCommandBuffer(commandBuffer);
				}

				/// <summary> Creates a primary command buffer </summary>
				virtual Reference<PrimaryCommandBuffer> CreatePrimaryCommandBuffer() override;

				/// <summary>
				/// Creates a bounch of primary command buffers
				/// </summary>
				/// <param name="count"> Number of command buffers to instantiate </param>
				/// <returns> List of command buffers </returns>
				virtual std::vector<Reference<PrimaryCommandBuffer>> CreatePrimaryCommandBuffers(size_t count) override;


				/// <summary> Creates a secondary command buffer </summary>
				virtual Reference<SecondaryCommandBuffer> CreateSecondaryCommandBuffer() override;

				/// <summary>
				/// Creates a bounch of secondary command buffers
				/// </summary>
				/// <param name="count"> Number of command buffers to instantiate </param>
				/// <returns> List of command buffers </returns>
				virtual std::vector<Reference<SecondaryCommandBuffer>> CreateSecondaryCommandBuffers(size_t count) override;


			private:
				// "Owener" device
				const Reference<VulkanDeviceQueue> m_queue;

				// Pool create flags
				const VkCommandPoolCreateFlags m_createFlags;

				// Underlying command pool
				const VkCommandPool m_commandPool;

				// Lock for out of scope command buffer collection
				mutable std::mutex m_outOfScopeLock;

				// Collection of command buffers, that went out of scope
				mutable std::vector<VkCommandBuffer> m_outOfScopeBuffers;

				// Frees all command buffers within m_outOfScopeBuffers collection
				void FreeOutOfScopeCommandBuffers()const;
			};
		}
	}
}
