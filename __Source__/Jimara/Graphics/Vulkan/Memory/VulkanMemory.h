#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanMemoryPool;
			class VulkanMemoryAllocation;
		}
	}
}
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan memory pool, responsible for memory allocations
			/// </summary>
			class VulkanMemoryPool {
			public:
				/// <summary>
				/// Allocates vulkan memory
				/// </summary>
				/// <param name="requirements"> Memory requirements </param>
				/// <param name="properties"> Required memory property flags </param>
				/// <returns> New vulkan memory allocation </returns>
				Reference<VulkanMemoryAllocation> Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)const;

				/// <summary> "Owner" graphics device </summary>
				VulkanDevice* GraphicsDevice()const;



			private:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" graphics device </param>
				VulkanMemoryPool(VulkanDevice* device);

				/// <summary> Destructor </summary>
				~VulkanMemoryPool();

				// VulkanDevice should be allowed to instantiate VulkanMemoryPool and VulkanMemoryAllocation needs access to it's internals
				friend class VulkanDevice;
				friend class VulkanMemoryAllocation;

				// "Owner" graphics device
				VulkanDevice* m_device;

				// Number of memory types
				size_t m_memoryTypeCount;

				// Underlying allocation data
				void* m_memoryTypePools;

				// We don't need someone playing around, copying class that's not supposed to be copied
				VulkanMemoryPool(const VulkanMemoryPool&) = delete;
				VulkanMemoryPool& operator=(const VulkanMemoryPool&) = delete;
			};



			/// <summary>
			/// Vulkan memory allocation
			/// </summary>
			class VulkanMemoryAllocation {
			public:
				/// <summary> Increaments reference counter </summary>
				void AddRef()const;

				/// <summary> Decrements reference counter </summary>
				void ReleaseRef()const;
				
				/// <summary> Allocation size </summary>
				VkDeviceSize Size()const;

				/// <summary> Device memory reference </summary>
				VkDeviceMemory Memory()const;

				/// <summary> Device memory offset </summary>
				VkDeviceSize Offset()const;

				/// <summary> Memory property flags (may and likely will contain some characteristics beyond ones requested during allocation) </summary>
				VkMemoryPropertyFlags Flags()const;

				/// <summary>
				/// Maps memory data to CPU buffer
				/// (Note: NEEDS corresponding Unmap() call)
				/// </summary>
				/// <param name="read"> If true, cpu buffer will not ignore initial content of the memory allocation </param>
				/// <returns> Mapped memory </returns>
				void* Map(bool read)const;

				/// <summary>
				/// Unmaps memory data
				/// </summary>
				/// <param name="write"> If true, GPU buffer will be updated with the content from the mapping, previously returned by Map() call </param>
				void Unmap(bool write)const;


			private:
				// Allocation reference counter
				mutable std::atomic<std::size_t> m_referenceCount;

				// 'Owner' memory pool
				const VulkanMemoryPool* m_memoryPool;
				
				// Memory type
				uint32_t m_memoryTypeId;

				// Memory block pool id (same block pool Id means similarly sized allocations, no matter the type id)
				size_t m_blockPoolId;

				// Memory block id within the block pool (block is a set of batch allocations of similarely sized memories)
				size_t m_memoryBlockId;

				// Memory allocation id within the memory block
				size_t m_blockAllocationId;

				// Memory type properties
				VkMemoryPropertyFlags m_flags;
				
				// Vulkan memory
				VkDeviceMemory m_memory;
				
				// Vulkan memory offset
				volatile VkDeviceSize m_offset;

				// Allocation size
				volatile VkDeviceSize m_size;

				// VulkanMemoryPool has to access constructor and alike
				friend class VulkanMemoryPool;

				/// <summary> Constructor </summary>
				VulkanMemoryAllocation();

				// We should not be able to copy the VulkanMemoryAllocation objects as we please...
				VulkanMemoryAllocation(const VulkanMemoryAllocation&) = delete;
				VulkanMemoryAllocation& operator=(const VulkanMemoryAllocation&) = delete;
			};
		}
	}
}
