#include "VulkanMemory.h"
#include <queue>


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static uint32_t FindMemoryType(VulkanDevice* device, uint32_t typeFilter, VkMemoryPropertyFlags& properties) {
					const VkPhysicalDeviceMemoryProperties& memoryProperties = device->PhysicalDeviceInfo()->MemoryProperties();
					for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
						if ((typeFilter & (1 << i)) != 0 && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
							properties = memoryProperties.memoryTypes[i].propertyFlags;
							return i;
						}
					device->Log()->Fatal("VulkanMemoryAllocation - Failed to find suitable memory type!");
				}

#define HEAP_POOL_MIN_BLOCK_SIZE_LOG 4
#define HEAP_POOL_MAX_BLOCK_POOLS ((sizeof(VkDeviceSize) << 3) - HEAP_POOL_MIN_BLOCK_SIZE_LOG)
#define BLOCK_POOL_MAX_ALLOCATIONS HEAP_POOL_MAX_BLOCK_POOLS

#define BLOCK_POOL_ALLOCATION_SIZE(blockPoolIndex) (static_cast<VkDeviceSize>(1) << static_cast<VkDeviceSize>(static_cast<VkDeviceSize>(blockPoolIndex) + HEAP_POOL_MIN_BLOCK_SIZE_LOG))
//#define BLOCK_ALLOCATION_COUNT(memoryBlockIndex) (static_cast<VkDeviceSize>(1) << static_cast<VkDeviceSize>(memoryBlockIndex))
//#define BLOCK_ALLOCATION_SIZE(blockPoolIndex, memoryBlockIndex) (BLOCK_POOL_ALLOCATION_SIZE(blockPoolIndex) * BLOCK_ALLOCATION_COUNT(memoryBlockIndex))

				struct BlockAllocation {
					VkDeviceMemory memoryBlock;
					VulkanMemoryAllocation* allocations;
					void* memoryMapping;

					BlockAllocation() : memoryBlock(VK_NULL_HANDLE), allocations(nullptr), memoryMapping(nullptr) {}
				};

				struct BlockPool {
					std::mutex lock;
					BlockAllocation memoryBlocks[BLOCK_POOL_MAX_ALLOCATIONS];
					std::queue<VulkanMemoryAllocation*> freeAllocations;

					Reference<VulkanMemoryAllocation> Allocate(const VulkanMemoryPool* pool, uint32_t memoryTypeIndex, size_t blockPoolId
						, VulkanMemoryAllocation* (*createMemoryAllocations)(
							const VulkanMemoryPool* device, uint32_t memoryTypeId, size_t blockPoolId, size_t blockId, VkDeviceSize allocationCount)) {
						std::unique_lock<std::mutex> allocationLock(lock);
						if (freeAllocations.size() > 0) {
							Reference<VulkanMemoryAllocation> allocation = freeAllocations.front();
							freeAllocations.pop();
							return allocation;
						}
						for (size_t blockId = 0; blockId < BLOCK_POOL_MAX_ALLOCATIONS; blockId++) {
							BlockAllocation& allocation = memoryBlocks[blockId];
							if (allocation.memoryBlock == VK_NULL_HANDLE) {
								VkDeviceSize sizePerBlock = BLOCK_POOL_ALLOCATION_SIZE(blockPoolId);
								VkDeviceSize allocationCount;
								for (allocationCount = (static_cast<VkDeviceSize>(1) << static_cast<VkDeviceSize>(blockId)); allocationCount > 0; (allocationCount >>= 1)) {
									VkMemoryAllocateInfo allocInfo = {};
									{
										allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
										allocInfo.allocationSize = sizePerBlock * allocationCount;
										allocInfo.memoryTypeIndex = memoryTypeIndex;
									}
									if (vkAllocateMemory(*pool->GraphicsDevice(), &allocInfo, nullptr, &allocation.memoryBlock) != VK_SUCCESS)
										allocation.memoryBlock = VK_NULL_HANDLE;
									else break;
								}
								if (allocation.memoryBlock == VK_NULL_HANDLE) return nullptr;
								
								allocation.allocations = createMemoryAllocations(pool, memoryTypeIndex, blockPoolId, blockId, allocationCount);
								for (VkDeviceSize i = 1; i < allocationCount; i++)
									freeAllocations.push(allocation.allocations + i);

								return allocation.allocations;
							}
						}
						pool->GraphicsDevice()->Log()->Fatal("VulkanMemoryPool - Failed to find appropriate memory block");
						return nullptr;
					}
				};

				struct MemoryTypePool {
					VkMemoryPropertyFlags properties;
					BlockPool blockPools[HEAP_POOL_MAX_BLOCK_POOLS];

					MemoryTypePool() : properties(0) {}

					Reference<VulkanMemoryAllocation> Allocate(const VulkanMemoryPool* pool, const VkMemoryRequirements& requirements, uint32_t memoryTypeIndex
						, VulkanMemoryAllocation* (*createMemoryAllocations)(
							const VulkanMemoryPool* device, uint32_t memoryTypeId, size_t blockPoolId, size_t blockId, VkDeviceSize allocationCount)) {

						for (size_t blockPoolId = 0; blockPoolId < HEAP_POOL_MAX_BLOCK_POOLS; blockPoolId++) {

							VkDeviceSize blockSize = BLOCK_POOL_ALLOCATION_SIZE(blockPoolId);

							if (blockSize < requirements.alignment || blockSize < requirements.size) continue;
							else if ((blockSize % requirements.alignment) == 0 || (blockSize >= (requirements.alignment + requirements.size)))
								return blockPools[blockPoolId].Allocate(pool, memoryTypeIndex, blockPoolId, createMemoryAllocations);
						}

						pool->GraphicsDevice()->Log()->Fatal("VulkanMemoryPool - Failed to find approptiate block size");
						return nullptr;
					}
				};
			}

			Reference<VulkanMemoryAllocation> VulkanMemoryPool::Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)const {
				MemoryTypePool* memoryTypePools = reinterpret_cast<MemoryTypePool*>(m_memoryTypePools);
				bool memoryTypeFound = false;
				if (memoryTypePools == nullptr)
					GraphicsDevice()->Log()->Fatal("VulkanMemoryPool - Device has no memory");
				else for (uint32_t memoryTypeId = 0; memoryTypeId < m_memoryTypeCount; memoryTypeId++) {
					MemoryTypePool& memoryTypePool = memoryTypePools[memoryTypeId];
					if ((requirements.memoryTypeBits & (1 << memoryTypeId)) != 0 && (memoryTypePool.properties & properties) == properties) {
						memoryTypeFound = true;
						Reference<VulkanMemoryAllocation> allocation = memoryTypePool.Allocate(this, requirements, memoryTypeId
							, [](const VulkanMemoryPool* pool, uint32_t memoryTypeId, size_t blockPoolId, size_t blockId, VkDeviceSize allocationCount) {

								MemoryTypePool& memoryTypePool = reinterpret_cast<MemoryTypePool*>(pool->m_memoryTypePools)[memoryTypeId];
								BlockAllocation& blockAllocation = memoryTypePool.blockPools[blockPoolId].memoryBlocks[blockId];
								VkDeviceMemory memory = blockAllocation.memoryBlock;

								VulkanMemoryAllocation* allocations = new VulkanMemoryAllocation[static_cast<size_t>(allocationCount)];
								for (VkDeviceSize i = 0; i < allocationCount; i++) {
									VulkanMemoryAllocation& allocation = allocations[i];
									allocation.m_memoryTypeId = memoryTypeId;
									allocation.m_blockPoolId = blockPoolId;
									allocation.m_memoryBlockId = blockId;
									allocation.m_blockAllocationId = static_cast<size_t>(i);

									allocation.m_memoryPool = pool;
									allocation.m_flags = memoryTypePool.properties;
									allocation.m_memory = memory;
								}
								if ((memoryTypePool.properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
									if (vkMapMemory(*pool->GraphicsDevice(), memory, 0, 
										allocationCount * BLOCK_POOL_ALLOCATION_SIZE(blockPoolId), 
										0, &blockAllocation.memoryMapping) != VK_SUCCESS)
										pool->GraphicsDevice()->Log()->Fatal("VulkanMemoryPool - Failed to map memory");
								return allocations;
							});
						if (allocation == nullptr) continue;
						m_device->AddRef();
						{
							VkDeviceSize allocationSize = BLOCK_POOL_ALLOCATION_SIZE(allocation->m_blockPoolId);
							VkDeviceSize blockOffset = allocationSize * allocation->m_blockAllocationId;
							VkDeviceSize remainder = (blockOffset % requirements.alignment);
							VkDeviceSize sizeLoss;
							if (remainder > 0) {
								sizeLoss = (requirements.alignment - remainder);
								blockOffset += sizeLoss;
							}
							else sizeLoss = 0;
							allocation->m_offset = blockOffset;
							allocation->m_size = (allocationSize - sizeLoss);
						}
						return allocation;
					}
				}
				GraphicsDevice()->Log()->Fatal(memoryTypeFound ? 
					"VulkanMemoryPool - Failed to allocate memory!" :
					"VulkanMemoryPool - Failed to find suitable memory type!");
				return nullptr;
			}

			VulkanDevice* VulkanMemoryPool::GraphicsDevice()const { return m_device; }

			VulkanMemoryPool::VulkanMemoryPool(VulkanDevice* device)
				: m_device(device), m_memoryTypeCount(device->PhysicalDeviceInfo()->MemoryProperties().memoryTypeCount), m_memoryTypePools(nullptr) {
				if (m_memoryTypeCount > 0) {
					MemoryTypePool* memoryTypePools = new MemoryTypePool[m_memoryTypeCount];
					const VkPhysicalDeviceMemoryProperties& memoryProperties = m_device->PhysicalDeviceInfo()->MemoryProperties();
					for (size_t memoryTypeId = 0; memoryTypeId < m_memoryTypeCount; memoryTypeId++)
						memoryTypePools[memoryTypeId].properties = memoryProperties.memoryTypes[memoryTypeId].propertyFlags;
					m_memoryTypePools = reinterpret_cast<void*>(memoryTypePools);
				}
			}

			VulkanMemoryPool::~VulkanMemoryPool() {
				MemoryTypePool* memoryTypePools = reinterpret_cast<MemoryTypePool*>(m_memoryTypePools);
				if (memoryTypePools != nullptr) {
					for (size_t memoryTypeId = 0; memoryTypeId < m_memoryTypeCount; memoryTypeId++) {
						MemoryTypePool& memoryTypePool = memoryTypePools[memoryTypeId];
						for (size_t blockPoolId = 0; blockPoolId < HEAP_POOL_MAX_BLOCK_POOLS; blockPoolId++) {
							BlockPool& blockPool = memoryTypePool.blockPools[blockPoolId];
							for (size_t allocationId = 0; allocationId < BLOCK_POOL_MAX_ALLOCATIONS; allocationId++) {
								BlockAllocation& blockAllocation = blockPool.memoryBlocks[allocationId];
								if (blockAllocation.memoryMapping != nullptr) {
									vkUnmapMemory(*m_device, blockAllocation.memoryBlock);
									blockAllocation.memoryMapping = nullptr;
								}
								if (blockAllocation.memoryBlock != VK_NULL_HANDLE) {
									vkFreeMemory(*m_device, blockAllocation.memoryBlock, nullptr);
									blockAllocation.memoryBlock = VK_NULL_HANDLE;
								}
								if (blockAllocation.allocations != nullptr) {
									delete[] blockAllocation.allocations;
									blockAllocation.allocations = nullptr;
								}
							}
						}
					}
					delete[] memoryTypePools;
					m_memoryTypePools = nullptr;
				}
			}

			VulkanMemoryAllocation::VulkanMemoryAllocation()
				: m_memoryPool(nullptr)
				, m_memoryTypeId(0), m_blockPoolId(0), m_memoryBlockId(0), m_blockAllocationId(0)
				, m_flags(0), m_memory(0), m_size(0), m_offset(0) {
				ReleaseRef();
			}

			VkDeviceSize VulkanMemoryAllocation::Size()const { return m_size; }

			VkMemoryPropertyFlags VulkanMemoryAllocation::Flags()const { return m_flags; }

			VkDeviceMemory VulkanMemoryAllocation::Memory()const { return m_memory; }

			VkDeviceSize VulkanMemoryAllocation::Offset()const { return m_offset; }

			void* VulkanMemoryAllocation::Map(bool read)const {
				void* data = reinterpret_cast<MemoryTypePool*>(m_memoryPool->m_memoryTypePools)[m_memoryTypeId].blockPools[m_blockPoolId].memoryBlocks[m_memoryBlockId].memoryMapping;
				if (data == nullptr) {
					m_memoryPool->GraphicsDevice()->Log()->Fatal("VulkanMemoryAllocation - Attempting to map memory invisible to host!");
					return nullptr;
				}
				else if (read && ((m_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)) {
					VkMappedMemoryRange range = {};
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.memory = Memory();
					range.offset = Offset();
					range.size = Size();
					if (vkInvalidateMappedMemoryRanges(*m_memoryPool->GraphicsDevice(), 1, &range) != VK_SUCCESS)
						m_memoryPool->GraphicsDevice()->Log()->Fatal("VulkanMemoryAllocation - Failed to invalidate memory ranges");
				}
				return static_cast<void*>(static_cast<uint8_t*>(data) + m_offset);
			}

			void VulkanMemoryAllocation::Unmap(bool write)const {
				void* data = reinterpret_cast<MemoryTypePool*>(m_memoryPool->m_memoryTypePools)[m_memoryTypeId].blockPools[m_blockPoolId].memoryBlocks[m_memoryBlockId].memoryMapping;
				if (data == nullptr) {
					m_memoryPool->GraphicsDevice()->Log()->Fatal("VulkanMemoryAllocation - Attempting to unmap memory invisible to host!");
					return;
				}
				else if (write && ((m_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)) {
					VkMappedMemoryRange range = {};
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.memory = Memory();
					range.offset = Offset();
					range.size = Size();
					if (vkFlushMappedMemoryRanges(*m_memoryPool->GraphicsDevice(), 1, &range) != VK_SUCCESS)
						m_memoryPool->GraphicsDevice()->Log()->Fatal("VulkanMemoryAllocation - Failed to flush memory ranges");
				}
			}

			void VulkanMemoryAllocation::OnOutOfScope()const {
				if (m_memoryPool == nullptr) return;
				BlockPool& blockPool = reinterpret_cast<MemoryTypePool*>(m_memoryPool->m_memoryTypePools)[m_memoryTypeId].blockPools[m_blockPoolId];
				std::unique_lock<std::mutex> lock(blockPool.lock);
				blockPool.freeAllocations.push((VulkanMemoryAllocation*)((void*)this));
				m_memoryPool->GraphicsDevice()->ReleaseRef();
			}
		}
	}
}
#pragma warning(default: 26812)
