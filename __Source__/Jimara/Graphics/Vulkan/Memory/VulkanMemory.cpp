#include "VulkanMemory.h"
#include <queue>


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace Legacy {
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
									allocation.m_blockAllocationCount = static_cast<size_t>(allocationCount);

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
				, m_memoryTypeId(0u), m_blockPoolId(0u), m_memoryBlockId(0u)
				, m_blockAllocationId(0u), m_blockAllocationCount(0u)
				, m_flags(0u), m_memory(0u), m_size(0u), m_offset(0u) {
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
				else if (
					((m_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) &&
					write) {
					const VkDeviceSize allocationSize = BLOCK_POOL_ALLOCATION_SIZE(m_blockPoolId);
					const VkDeviceSize blockSize = VkDeviceSize(m_blockAllocationCount * allocationSize);
					const VkDeviceSize atomSize = m_memoryPool->m_device->PhysicalDeviceInfo()->DeviceProperties().limits.nonCoherentAtomSize;
					const VkDeviceSize offset = (Offset() / atomSize) * atomSize;
					const VkDeviceSize chunkSize = ((Offset() + Size() - offset + atomSize - 1u) / atomSize) * atomSize;

					VkMappedMemoryRange range = {};
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.memory = Memory();
					range.offset = offset;
					range.size = ((offset + chunkSize) >= blockSize) ? VK_WHOLE_SIZE : chunkSize;
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
		




		namespace Refactor {
		struct VulkanMemoryPool::Helpers {
			static std::recursive_mutex& DefaultAllocationLock() {
				static thread_local std::recursive_mutex lock;
				return lock;
			}

			struct AllocationGroup : public virtual Object {
				const VkDeviceMemory vulkanMemory;
				void* const mappedMemory;
				const VkDeviceSize sizePerAllocation;
				const VkMemoryPropertyFlags propertyFlags;
				const Reference<VkDeviceHandle> device;
				const Reference<OS::Logger> log;
				const Reference<MemoryTypeSubpool> subpool;

				struct AllocationBlob {
					uint8_t allocationData[sizeof(VulkanMemoryAllocation)];
				};
				static_assert(sizeof(AllocationBlob) == sizeof(VulkanMemoryAllocation));
				std::vector<AllocationBlob> allocations;
				std::vector<VulkanMemoryAllocation*> freeAllocations;

				inline VulkanMemoryAllocation* AllocationData() {
					return reinterpret_cast<VulkanMemoryAllocation*>(allocations.data());
				}

				inline AllocationGroup(
					VkDeviceHandle* deviceHandle, OS::Logger* logger, MemoryTypeSubpool* memorySubpool,
					VkDeviceMemory memory, void* mapped, VkMemoryPropertyFlags flags,
					VkDeviceSize allocationSize, VkDeviceSize numAllocations) 
					: vulkanMemory(memory)
					, mappedMemory(mapped)
					, sizePerAllocation(allocationSize)
					, propertyFlags(flags)
					, device(deviceHandle)
					, log(logger)
					, subpool(memorySubpool)
					, allocations(static_cast<size_t>(numAllocations)) {
					for (size_t i = 0u; i < allocations.size(); i++) {
						VulkanMemoryAllocation& alloc = AllocationData()[i];
						new (&alloc) VulkanMemoryAllocation();
						alloc.m_allocationGroup = this;
						alloc.ReleaseRef();
						assert(alloc.m_allocationGroup == nullptr);
						assert(alloc.RefCount() == 0u);
						assert(freeAllocations.size() == (i + 1u));
					}
				}

				inline virtual ~AllocationGroup() {
					if (mappedMemory != nullptr)
						vkUnmapMemory(*device, vulkanMemory);
					vkFreeMemory(*device, vulkanMemory, nullptr);
					for (size_t i = 0u; i < allocations.size(); i++)
						(AllocationData() + i)->~VulkanMemoryAllocation();
				}

				inline Reference<VulkanMemoryAllocation> Allocate(VkDeviceSize alignment, VkDeviceSize size) {
					// Sanity check:
					if (size > sizePerAllocation) {
						log->Error(
							"VulkanMemoryPool::Helpers::AllocationGroup::Allocate - Sub-allocation can not be larger than sizePerAllocation! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}

					// Obtain free chunk:
					std::unique_lock<std::recursive_mutex> lock((subpool == nullptr) ? DefaultAllocationLock() : subpool->lock);
					if (freeAllocations.size() <= 0u)
						return nullptr;
					Reference<VulkanMemoryAllocation> result = freeAllocations.back();

					// Calculate and check offset and size:
					const VkDeviceSize chunkStart = (result.operator->() - AllocationData()) * sizePerAllocation;
					const VkDeviceSize chunkEnd = chunkStart + sizePerAllocation;
					alignment = Math::Max(alignment, VkDeviceSize(1u));
					const VkDeviceSize alignedOffset = ((chunkStart + alignment - 1u) / alignment) * alignment;
					if (alignedOffset > (chunkEnd - size)) {
						log->Error(
							"VulkanMemoryPool::Helpers::AllocationGroup::Allocate - Aligned sub-allocation can not fit in the dedicated memory segment! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}

					// Finish allocation:
					result->m_allocationGroup = this;
					result->m_offset = alignedOffset;
					result->m_size = size;
					freeAllocations.pop_back();
					assert(result->RefCount() == 1u);
					if (subpool != nullptr && freeAllocations.empty())
						subpool->groups.erase(this);
					return result;
				}

				inline static Reference<AllocationGroup> Create(
					VkDeviceHandle* device, OS::Logger* log, MemoryTypeSubpool* subpool,
					VkDeviceSize allocationSize, VkDeviceSize numAllocations, uint32_t memoryTypeIndex) {
					assert(log != nullptr);
					assert(device != nullptr);
					assert(memoryTypeIndex < device->PhysicalDevice()->MemoryProperties().memoryTypeCount);
					auto fail = [&](const auto&... message) { 
						log->Error("VulkanMemoryPool::Helpers::AllocationGroup::Create - ", message...); 
						return nullptr;
					};

					// Allocate memory:
					VkDeviceMemory vulkanMemory = VK_NULL_HANDLE;
					{
						VkMemoryAllocateInfo allocInfo = {};
						allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
						allocInfo.allocationSize = allocationSize * numAllocations;
						allocInfo.memoryTypeIndex = memoryTypeIndex;
						VkResult result = vkAllocateMemory(*device, &allocInfo, nullptr, &vulkanMemory);
						if (result != VK_SUCCESS)
							return fail("Failed to allocate memory (error code: ", result, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					// Map memory if it is host-visible:
					void* mappedMemory = nullptr;
					const VkMemoryPropertyFlags propertyFlags = device->PhysicalDevice()->MemoryProperties().memoryTypes[memoryTypeIndex].propertyFlags;
					if ((propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u) {
						VkResult result = vkMapMemory(*device, vulkanMemory, 0, allocationSize * numAllocations, 0u, &mappedMemory);
						if (result != VK_SUCCESS) {
							vkFreeMemory(*device, vulkanMemory, nullptr);
							return fail("Failed to map memory (error code: ", result, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}

					// Create group:
					return Object::Instantiate<AllocationGroup>(
						device, log, subpool, vulkanMemory, mappedMemory, propertyFlags, allocationSize, numAllocations);
				}
			};

			inline static VkMappedMemoryRange GetNonCoherentMappedMemoryRange(const VulkanMemoryAllocation* allocation) {
				AllocationGroup* group = dynamic_cast<AllocationGroup*>(allocation->m_allocationGroup.operator->());
				assert(group != nullptr);

				// Basic boundaries:
				const VkDeviceSize atomSize = Math::Max(group->device->PhysicalDevice()->DeviceProperties().limits.nonCoherentAtomSize, VkDeviceSize(1u));
				const VkDeviceSize allocationIndex = static_cast<VkDeviceSize>(allocation - group->AllocationData());
				const VkDeviceSize allocationBase = allocationIndex * group->sizePerAllocation;
				const VkDeviceSize allocationEnd = allocationBase + group->sizePerAllocation;
				const VkDeviceSize memoryStart = allocation->Offset();
				const VkDeviceSize memoryEnd = memoryStart + allocation->Size();
				const VkDeviceSize atomAlignedStart = (memoryStart / atomSize) * atomSize;
				const VkDeviceSize atomAlignedEnd = ((memoryEnd + atomSize - 1u) / atomSize) * atomSize;
				
				// Warn if chunks overlap in any way:
				if (atomAlignedStart < memoryStart)
					group->log->Warning(
						"VulkanMemoryPool::Helpers::GetMappedMemoryRange - Can not isolate VkMappedMemoryRange that does not overlap with previous chunk. ",
						"This may result in unsafe behaviour! [File:", __FILE__, "; Line: ", __LINE__, "]");
				if (atomAlignedEnd > memoryEnd && group->allocations.size() > 1u)
					group->log->Warning(
						"VulkanMemoryPool::Helpers::GetMappedMemoryRange - Can not isolate VkMappedMemoryRange that does not overlap with next chunk. ",
						"This may result in unsafe behaviour! [File:", __FILE__, "; Line: ", __LINE__, "]");

				// Return range:
				VkMappedMemoryRange range = {};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.pNext = nullptr;
				range.memory = group->vulkanMemory;
				range.offset = atomAlignedStart;
				range.size = (atomAlignedEnd <= memoryEnd) ? (atomAlignedEnd - atomAlignedStart) : VK_WHOLE_SIZE;
				return range;
			}
		};

		VulkanMemoryPool::VulkanMemoryPool(VulkanDevice* device) 
			: m_deviceHandle(*device)
			, m_logger(device->Log()) {
			assert(m_deviceHandle != nullptr);
			assert(m_logger != nullptr);
			const VkPhysicalDeviceMemoryProperties& memoryProps = m_deviceHandle->PhysicalDevice()->MemoryProperties();
			const VkPhysicalDeviceLimits& deviceLimits = m_deviceHandle->PhysicalDevice()->DeviceProperties().limits;
			m_subpools.resize(memoryProps.memoryTypeCount);
			for (size_t i = 0u; i < m_subpools.size(); i++) {
				MemoryTypeSubpools& subpools = m_subpools[i];
				for (size_t j = 0u; j < 64u; j++)
					subpools.push_back(Object::Instantiate<MemoryTypeSubpool>());
			}
			VkDeviceSize vramCapacity = static_cast<VkDeviceSize>(m_deviceHandle->PhysicalDevice()->VramCapacity());
			m_individualAllocationThreshold = Math::Max(
				vramCapacity / Math::Min(static_cast<VkDeviceSize>(deviceLimits.maxMemoryAllocationCount), VkDeviceSize(256u)),
				VkDeviceSize(256u));
		}

		VulkanMemoryPool::~VulkanMemoryPool() {}

		Reference<VulkanMemoryAllocation> VulkanMemoryPool::Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)const {
			const VkPhysicalDeviceMemoryProperties& memoryProps = m_deviceHandle->PhysicalDevice()->MemoryProperties();
			const VkDeviceSize alignment = Math::Max(requirements.alignment, VkDeviceSize(1u));

			for (decltype(memoryProps.memoryTypeCount) memoryTypeId = 0u; memoryTypeId < memoryProps.memoryTypeCount; memoryTypeId++) {
				// If memory type is incompatible, we should ignore it:
				const VkMemoryType& memoryType = memoryProps.memoryTypes[memoryTypeId];
				if ((requirements.memoryTypeBits & (1 << memoryTypeId)) == 0u ||
					(memoryType.propertyFlags & properties) != properties)
					continue;

				// If size is small enough, we try to sub-allocate:
				if (requirements.size <= m_individualAllocationThreshold) {
					const VkDeviceSize minChunkSize = (
						((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0u) &&
						((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u))
						? m_deviceHandle->PhysicalDevice()->DeviceProperties().limits.nonCoherentAtomSize : VkDeviceSize(32u);
					const VkDeviceSize groupAllocationThreshold = m_individualAllocationThreshold << 1u;

					const MemoryTypeSubpools& subpools = m_subpools[memoryTypeId];
					for (size_t subpoolId = 0u; subpoolId < subpools.size(); subpoolId++) {
						// Check if the subpool chunk size is large enough:
						const VkDeviceSize chunkSize = minChunkSize << subpoolId;
						const VkDeviceSize requiredChunkSize = requirements.size +
							(((chunkSize % alignment) == 0u)
								? VkDeviceSize(0u) : VkDeviceSize(alignment - 1u));
						if (chunkSize < requiredChunkSize)
							continue;

						// Try to allocate within the subpool:
						MemoryTypeSubpool* subpool = subpools[subpoolId];
						{
							std::unique_lock<std::recursive_mutex> lock(subpool->lock);

							// Create new group if there are no free groups:
							if (subpool->groups.empty()) {
								subpool->maxGroupSize <<= 1u;
								if ((chunkSize * subpool->maxGroupSize) > groupAllocationThreshold)
									subpool->maxGroupSize >>= 1u;
								Reference<Helpers::AllocationGroup> group = Helpers::AllocationGroup::Create(
									m_deviceHandle, m_logger, subpool, chunkSize, subpool->maxGroupSize, memoryTypeId);
								if (group != nullptr)
									subpool->groups.insert(group);
							}

							// Return allocation if we have valid subgroups:
							if (!subpool->groups.empty()) {
								Reference<Helpers::AllocationGroup> group = *subpool->groups.begin();
								Reference<VulkanMemoryAllocation> allocation = group->Allocate(alignment, requirements.size);
								if (allocation == nullptr)
									m_logger->Fatal(
										"VulkanMemoryPool - Failed to sub-allocate memory from existing group! ", 
										"[File: ", __FILE__, "; Line: ", __LINE__, "]");
								return allocation;
							}
						}
					}
				}

				// If we got here, let us try to make a single allocation:
				Reference<Helpers::AllocationGroup> allocationGroup = Helpers::AllocationGroup::Create(
					m_deviceHandle, m_logger, nullptr, requirements.size, 1u, memoryTypeId);
				if (allocationGroup != nullptr) {
					Reference<VulkanMemoryAllocation> allocation = allocationGroup->Allocate(alignment, requirements.size);
					if (allocation == nullptr)
						m_logger->Fatal(
							"VulkanMemoryPool - Failed to allocate memory from a new group! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return allocation;
				}
			}

			// If we got here, we failed:
			m_logger->Error("VulkanMemoryPool - Failed to find memory type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}


		VkDeviceSize VulkanMemoryAllocation::Size()const { return m_size; }

		VkDeviceMemory VulkanMemoryAllocation::Memory()const {
			assert(m_allocationGroup != nullptr);
			return dynamic_cast<VulkanMemoryPool::Helpers::AllocationGroup*>(m_allocationGroup.operator->())->vulkanMemory;
		}

		VkDeviceSize VulkanMemoryAllocation::Offset()const { return m_offset; }

		VkMemoryPropertyFlags VulkanMemoryAllocation::Flags()const {
			assert(m_allocationGroup != nullptr);
			return dynamic_cast<VulkanMemoryPool::Helpers::AllocationGroup*>(m_allocationGroup.operator->())->propertyFlags;
		}

		void* VulkanMemoryAllocation::Map(bool read)const {
			VulkanMemoryPool::Helpers::AllocationGroup* group = dynamic_cast<VulkanMemoryPool::Helpers::AllocationGroup*>(m_allocationGroup.operator->());
			assert(group != nullptr);
			void* data = group->mappedMemory;
			if (data == nullptr) {
				group->log->Error("VulkanMemoryAllocation::Map - Attempting to map memory invisible to host! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			if (read && ((group->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)) {
				const VkMappedMemoryRange range = VulkanMemoryPool::Helpers::GetNonCoherentMappedMemoryRange(this);
				VkResult result = vkInvalidateMappedMemoryRanges(*group->device, 1u, &range);
				if (result != VK_SUCCESS)
					group->log->Error(
						"VulkanMemoryAllocation::Map - Failed to invalidate memory ranges (error code: ", result, ")! "
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
			return static_cast<void*>(static_cast<uint8_t*>(data) + Offset());
		}

		void VulkanMemoryAllocation::Unmap(bool write)const {
			VulkanMemoryPool::Helpers::AllocationGroup* group = dynamic_cast<VulkanMemoryPool::Helpers::AllocationGroup*>(m_allocationGroup.operator->());
			assert(group != nullptr);
			void* data = group->mappedMemory;
			if (data == nullptr) {
				group->log->Error("VulkanMemoryAllocation::Unmap - Attempting to unmap memory invisible to host! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			if (write && ((group->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)) {
				const VkMappedMemoryRange range = VulkanMemoryPool::Helpers::GetNonCoherentMappedMemoryRange(this);
				VkResult result = vkFlushMappedMemoryRanges(*group->device, 1u, &range);
				if(result != VK_SUCCESS)
					group->log->Error(
						"VulkanMemoryAllocation::Unmap - Failed to flush memory ranges (error code: ", result, ")! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		}

		void VulkanMemoryAllocation::OnOutOfScope()const {
			Reference<VulkanMemoryPool::Helpers::AllocationGroup> group = m_allocationGroup;
			assert(group != nullptr);
			{
				std::unique_lock<std::recursive_mutex> lock(
					group->subpool == nullptr ? VulkanMemoryPool::Helpers::DefaultAllocationLock() : group->subpool->lock);
				VulkanMemoryAllocation* self = group->AllocationData() + (this - group->AllocationData());
				assert(self == this);
				self->m_allocationGroup = nullptr;
				assert(m_allocationGroup == nullptr);
				group->freeAllocations.push_back(self);
				assert(RefCount() <= 0u);
				if (group->subpool != nullptr) {
					if (group->freeAllocations.size() == group->allocations.size())
						group->subpool->groups.erase(group);
					else if (group->freeAllocations.size() == 1u)
						group->subpool->groups.insert(group);
				}
			}
		}

		}
		}
	}
}
#pragma warning(default: 26812)
