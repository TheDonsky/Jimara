#pragma once
#include "VulkanCpuWriteOnlyBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			// Make sure DrawIndirectCommand and VkDrawIndexedIndirectCommand are binary equivalents. Otherwise, there's no chance our implementations work...
			static_assert(sizeof(DrawIndirectCommand) == sizeof(VkDrawIndexedIndirectCommand));
			static_assert(offsetof(DrawIndirectCommand, indexCount) == offsetof(VkDrawIndexedIndirectCommand, indexCount));
			static_assert(offsetof(DrawIndirectCommand, instanceCount) == offsetof(VkDrawIndexedIndirectCommand, instanceCount));
			static_assert(offsetof(DrawIndirectCommand, firstIndex) == offsetof(VkDrawIndexedIndirectCommand, firstIndex));
			static_assert(offsetof(DrawIndirectCommand, vertexOffset) == offsetof(VkDrawIndexedIndirectCommand, vertexOffset));
			static_assert(offsetof(DrawIndirectCommand, firstInstance) == offsetof(VkDrawIndexedIndirectCommand, firstInstance));
			
			/// <summary> Vulkan-specific implementation of Indirect draw buffer that can be read from and written to by a CPU/GPU  </summary>
			class JIMARA_API VulkanCPUReadWriteIndirectDrawBuffer : public virtual VulkanArrayBuffer, public virtual IndirectDrawBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="objectCount"> Number of entries within the buffer </param>
				inline VulkanCPUReadWriteIndirectDrawBuffer(VulkanDevice* device, size_t objectCount)
					: VulkanArrayBuffer(device, sizeof(DrawIndirectCommand), objectCount, false,
						VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {}
			};

			/// <summary> Vulkan-specific implementation of Indirect draw buffer that can be written to by the CPU and/or GPU and only be read by GPU </summary>
			class JIMARA_API VulkanCPUWriteOnlyIndirectDrawBuffer : public virtual VulkanCpuWriteOnlyBuffer, public virtual IndirectDrawBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="objectCount"> Number of entries within the buffer </param>
				inline VulkanCPUWriteOnlyIndirectDrawBuffer(VulkanDevice* device, size_t objectCount) 
					: VulkanArrayBuffer(
						device, sizeof(DrawIndirectCommand), objectCount, true,
						VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					, VulkanCpuWriteOnlyBuffer(device, sizeof(DrawIndirectCommand), objectCount) {
					if ((Usage() & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) == 0)
						Device()->Log()->Fatal("VulkanCPUWriteOnlyIndirectDrawBuffer::VulkanCPUWriteOnlyIndirectDrawBuffer - ",
							"VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT missing from usage flags! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}
			};
		}
	}
}
