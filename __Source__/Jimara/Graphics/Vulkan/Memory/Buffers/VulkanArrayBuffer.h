#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanArrayBuffer;
		}
	}
}
#include "../VulkanMemory.h"
#include "../../Pipeline/VulkanCommandBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed ArrayBuffer interface
			/// </summary>
			class JIMARA_API VulkanArrayBuffer : public virtual ArrayBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="objectSize"> Size of an individual element within the buffer </param>
				/// <param name="objectCount"> Count of elements within the buffer </param>
				/// <param name="writeOnly"> If true, Map() call will not bother with invalidating any mapped memory ranges, potentially speeding up mapping process and ignoring GPU-data </param>
				/// <param name="usage"> Buffer usage flags </param>
				/// <param name="memoryFlags"> Buffer memory flags </param>
				VulkanArrayBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, bool writeOnly, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanArrayBuffer();

				/// <summary> Size of an individual object/structure within the buffer </summary>
				virtual size_t ObjectSize()const override;

				/// <summary> Number of objects within the buffer </summary>
				virtual size_t ObjectCount()const override;

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

				/// <summary> "Owner" vulkan device </summary>
				VulkanDevice* Device()const;

				/// <summary> Buffer usage </summary>
				VkBufferUsageFlags Usage()const;

				/// <summary> Buffer memory flags </summary>
				VkMemoryPropertyFlags MemoryFlags()const;

				/// <summary> Memory allocation size </summary>
				VkDeviceSize AllocationSize()const;

				/// <summary>
				/// Gives the command buffer access to the underlying data
				/// </summary>
				/// <param name="commandBuffer"> Command buffer that relies on the resource </param>
				/// <returns> Underlying buffer </returns>
				virtual VkBuffer GetVulkanHandle(VulkanCommandBuffer* commandBuffer);



			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Size of an individual element within the buffer
				const size_t m_elemSize;

				// Count of elements within the buffer
				const size_t m_elemCount;

				// If true, Map() call will not bother with invalidating any mapped memory ranges, potentially speeding up mapping process and ignoring GPU-data
				const bool m_writeOnly;

				// Buffer usage flags
				const VkBufferUsageFlags m_usage;

				// Buffer memory flags
				const VkMemoryPropertyFlags m_memoryFlags;

				// Underlying API object
				VkBuffer m_buffer;

				// Buffer memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};
		}
	}
}
