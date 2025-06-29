#pragma once
#include "VulkanArrayBuffer.h"
#include "../../../../Core/Synch/SpinLock.h"
#include "../VulkanOneTimeCommandBufferCache.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan buffer that resides on GPU memory and maps to a staging buffer for writes
			/// Note: CPU_READ_WRITE is implemented, but I would not call it fully functional just yet, since you can still map the memory while in use by GPU <_TODO_>
			/// </summary>
			class JIMARA_API VulkanCpuWriteOnlyBuffer : public virtual VulkanArrayBuffer {
			public:
				/// <summary> Default usage flags </summary>
				static const constexpr VkBufferUsageFlags DEFAULT_USAGE_BASE =
					VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
					VK_BUFFER_USAGE_TRANSFER_DST_BIT |
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				/// <summary> Additional default usage flags when RT hardware is present </summary>
				static const constexpr VkBufferUsageFlags DEFAULT_USAGE_RT_ENABLED =
					VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;


				/// <summary>
				/// Default usage flags based on physical device capabilities
				/// </summary>
				/// <param name="device"> Physical device </param>
				/// <returns> Usage flags </returns>
				static VkBufferUsageFlags DefaultUsage(VulkanPhysicalDevice* device);

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="objectSize"> Size of an individual object within the buffer </param>
				/// <param name="objectCount"> Count of objects within the buffer </param>
				/// <param name="usage"> Usage flags </param>
				VulkanCpuWriteOnlyBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, VkBufferUsageFlags usage);

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="objectSize"> Size of an individual object within the buffer </param>
				/// <param name="objectCount"> Count of objects within the buffer </param>
				VulkanCpuWriteOnlyBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanCpuWriteOnlyBuffer();

				/// <summary> CPU access info </summary>
				virtual CPUAccess HostAccess()const override;

				/// <summary>
				/// Maps buffer memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during buffer creation(or buffer type when CPUAccess does not apply), 
				///		 the actual content of the buffer will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				virtual void* Map() override;

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				virtual void Unmap(bool write) override;


			private:
				// Lock for m_dataBuffer and m_stagingBuffer
				std::mutex m_bufferLock;

				// CPU-Mapped memory buffer
				Reference<VulkanArrayBuffer> m_stagingBuffer;

				// CPU-Mapped data
				std::atomic<void*> m_cpuMappedData;

				// Data updater
				VulkanOneTimeCommandBufferCache m_updateCache;
			};
		}
	}
}
