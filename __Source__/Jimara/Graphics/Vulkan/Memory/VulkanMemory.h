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
				struct MemoryTypeSubpool : public virtual Object {
					std::recursive_mutex lock;
					std::unordered_set<Reference<Object>> groups;
					VkDeviceSize maxGroupSize = 1u;

					inline virtual ~MemoryTypeSubpool() {
						assert(groups.empty());
					}
				};
				using MemoryTypeSubpools = std::vector<Reference<MemoryTypeSubpool>>;
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
