#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanArrayBuffer;
			class VulkanStaticBuffer;
		}
	}
}
#include "../VulkanMemory.h"
#include "../../Pipeline/VulkanCommandRecorder.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed ArrayBuffer interface
			/// </summary>
			class VulkanArrayBuffer : public virtual ArrayBuffer {
			public:
				/// <summary>
				/// Access data buffer
				/// </summary>
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the data buffer </returns>
				virtual Reference<VulkanStaticBuffer> GetStaticHandle(VulkanCommandRecorder* commandRecorder) = 0;
			};

			/// <summary> Basic wrapper on top of a VkBuffer </summary>
			class VulkanStaticBuffer : public virtual VulkanArrayBuffer {
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
				VulkanStaticBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, bool writeOnly, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanStaticBuffer();

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

				/// <summary> Buffer usage </summary>
				VkBufferUsageFlags Usage()const;

				/// <summary> Buffer memory flags </summary>
				VkMemoryPropertyFlags MemoryFlags()const;

				/// <summary> Type cast to API object </summary>
				operator VkBuffer()const;

				/// <summary> Memory allocation size </summary>
				VkDeviceSize AllocationSize()const;

				/// <summary>
				/// Access data buffer (self, in this case)
				/// </summary>
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the data buffer </returns>
				virtual Reference<VulkanStaticBuffer> GetStaticHandle(VulkanCommandRecorder* commandRecorder) override;



			private:
				// "Owner" device
				Reference<VulkanDevice> m_device;

				// Size of an individual element within the buffer
				size_t m_elemSize;

				// Count of elements within the buffer
				size_t m_elemCount;

				// If true, Map() call will not bother with invalidating any mapped memory ranges, potentially speeding up mapping process and ignoring GPU-data
				bool m_writeOnly;

				// Buffer usage flags
				VkBufferUsageFlags m_usage;

				// Buffer memory flags
				VkMemoryPropertyFlags m_memoryFlags;

				// Underlying API object
				VkBuffer m_buffer;

				// Buffer memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};
		}
	}
}
