#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace Legacy {
			class VulkanMemoryPool;
			class VulkanMemoryAllocation;
			}
			namespace Refactor {
			class VulkanMemoryPool;
			class VulkanMemoryAllocation;
			}

			/*
			using VulkanMemoryPool = Legacy::VulkanMemoryPool;
			using VulkanMemoryAllocation = Legacy::VulkanMemoryAllocation;
			/*/
			using VulkanMemoryPool = Refactor::VulkanMemoryPool;
			using VulkanMemoryAllocation = Refactor::VulkanMemoryAllocation;
			//*/
		}
	}
}
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace Legacy {
			/// <summary>
			/// Vulkan memory pool, responsible for memory allocations
			/// </summary>
			class JIMARA_API VulkanMemoryPool {
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
			class JIMARA_API VulkanMemoryAllocation : public virtual Object {
			public:
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


			protected:
				/// <summary>
				/// Invoked, when out of scope
				/// </summary>
				virtual void OnOutOfScope()const override;

			private:
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

				// Number of blocks in the allocation
				size_t m_blockAllocationCount;

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





			namespace Refactor {
			/// <summary>
			/// Vulkan memory pool, responsible for memory allocations
			/// </summary>
			class JIMARA_API VulkanMemoryPool {
			public:
				/// <summary>
				/// Allocates vulkan memory
				/// </summary>
				/// <param name="requirements"> Memory requirements </param>
				/// <param name="properties"> Required memory property flags </param>
				/// <returns> New vulkan memory allocation </returns>
				Reference<VulkanMemoryAllocation> Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)const;


			private:
				// Device handle:
				const Reference<VkDeviceHandle> m_deviceHandle;

				// Logger:
				const Reference<OS::Logger> m_logger;

				// Per-memory-type data:
				struct MemoryTypeSubpool {
					mutable std::mutex lock;
					mutable Reference<Object> group;
				};
				using MemoryTypeSubpools = std::vector<MemoryTypeSubpool>;
				std::vector<MemoryTypeSubpools> m_subpools;

				// Any object larger than this size will not be sub-allocated:
				VkDeviceSize m_individualAllocationThreshold = 0u;

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

				// We don't need someone playing around, copying class that's not supposed to be copied
				VulkanMemoryPool(const VulkanMemoryPool&) = delete;
				VulkanMemoryPool& operator=(const VulkanMemoryPool&) = delete;

				// Private stuff resides in here:
				struct Helpers;
			};


			/// <summary>
			/// Vulkan memory allocation
			/// </summary>
			class JIMARA_API VulkanMemoryAllocation : public virtual Object {
			public:
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

			protected:
				/// <summary>
				/// Invoked, when out of scope
				/// </summary>
				virtual void OnOutOfScope()const override;

			private:
				// Memory allocation group
				Reference<Object> m_allocationGroup;

				// Memory offset
				VkDeviceSize m_offset = 0u;

				// Memory size
				VkDeviceSize m_size = 0u;

				// Memory pool is able to access internals
				friend class VulkanMemoryPool;

				// Constructor
				inline VulkanMemoryAllocation() {}

				// We should not be able to copy the VulkanMemoryAllocation objects as we please...
				VulkanMemoryAllocation(const VulkanMemoryAllocation&) = delete;
				VulkanMemoryAllocation& operator=(const VulkanMemoryAllocation&) = delete;
			};
			}
		}
	}
}
