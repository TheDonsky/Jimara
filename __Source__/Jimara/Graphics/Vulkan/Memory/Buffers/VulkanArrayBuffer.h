#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanArrayBuffer;
		}
	}
}
#include "../VulkanMemory.h"
#include "../../Pipeline/Commands/VulkanCommandBuffer.h"


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

				/// <summary>
				/// Copies a region of given buffer into a region of this one
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record copy operation on </param>
				/// <param name="srcBuffer"> Source buffer </param>
				/// <param name="numBytes"> Number of bytes to copy (Element sizes do not have to match; this is size in bytes. if out of bounds size is requested, this number will be truncated) </param>
				/// <param name="dstOffset"> Index of the byte from this buffer to start writing at (Note: this indicates 'Byte', not element) </param>
				/// <param name="srcOffset"> Index of the byte from srcBuffer to start copying from (Note: this indicates 'Byte', not element) </param>
				virtual void Copy(CommandBuffer* commandBuffer, ArrayBuffer* srcBuffer, size_t numBytes = ~size_t(0), size_t dstOffset = 0u, size_t srcOffset = 0u) override;

				/// <summary> "Owner" vulkan device </summary>
				VulkanDevice* Device()const;

				/// <summary> Buffer usage </summary>
				VkBufferUsageFlags Usage()const;

				/// <summary> Buffer memory flags </summary>
				VkMemoryPropertyFlags MemoryFlags()const;

				/// <summary> Memory allocation size </summary>
				VkDeviceSize AllocationSize()const;

				/// <summary> Type-cast to underlying API object </summary>
				operator VkBuffer()const;

				/// <summary> Device address </summary>
				inline VkDeviceAddress VulkanDeviceAddress()const { return m_deviceAddress; }

				/// <summary> Device address of the buffer for buffer_reference </summary>
				inline virtual uint64_t DeviceAddress()const override {
					static_assert(sizeof(VkDeviceAddress) == sizeof(uint64_t));
					return static_cast<uint64_t>(VulkanDeviceAddress());
				}

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

				// Device address
				VkDeviceAddress m_deviceAddress = 0u;
				static_assert(std::is_integral_v<VkDeviceAddress>);
				static_assert(sizeof(VkDeviceAddress) <= sizeof(uint64_t));

				// Buffer memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};
		}
	}
}
