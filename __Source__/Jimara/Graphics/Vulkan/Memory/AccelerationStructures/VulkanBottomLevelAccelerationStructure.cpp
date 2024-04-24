#include "VulkanBottomLevelAccelerationStructure.h"



namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			struct VulkanBottomLevelAccelerationStructure::Helpers {
				inline static void FillBasicBuildInfo(
					const VulkanDevice* device, const BottomLevelAccelerationStructure::Properties& properties,
					VkAccelerationStructureBuildGeometryInfoKHR& buildInfo, VkAccelerationStructureGeometryKHR& geometry) {
					{
						buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
						buildInfo.pNext = nullptr;
						buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
						buildInfo.flags = VulkanAccelerationStructure::GetBuildFlags(properties.flags);

						// Ignored during creation:
						buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
						buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
						buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
						buildInfo.scratchData.hostAddress = nullptr;

						buildInfo.geometryCount = 1u;
						buildInfo.pGeometries = &geometry;
						buildInfo.ppGeometries = NULL;
					}

					{
						geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
						geometry.pNext = nullptr;
						geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
						geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

						geometry.geometry.triangles.pNext = nullptr;
						geometry.geometry.triangles.vertexFormat =
							(properties.vertexFormat == VertexFormat::X16Y16Z16)
							? VK_FORMAT_R16G16B16_SFLOAT
							: VK_FORMAT_R32G32B32_SFLOAT;
						geometry.geometry.triangles.vertexData.hostAddress = nullptr; // Ignored during creation
						geometry.geometry.triangles.vertexStride = 0; // __TODO__: Check if this is ignored during creation...
						geometry.geometry.triangles.maxVertex = properties.maxVertexCount;
						geometry.geometry.triangles.indexType =
							(properties.indexFormat == IndexFormat::U16)
							? VK_INDEX_TYPE_UINT16
							: VK_INDEX_TYPE_UINT32;

						// Ignored during creation:
						geometry.geometry.triangles.indexData.hostAddress = nullptr;
						geometry.geometry.triangles.transformData.hostAddress = nullptr;
					
						geometry.flags = VulkanAccelerationStructure::GetGeometryFlags(properties.flags);
					}
				}
			};

			Reference<VulkanBottomLevelAccelerationStructure> VulkanBottomLevelAccelerationStructure::Create(
				VulkanDevice* device, const BottomLevelAccelerationStructure::Properties& properties) {
				if (device == nullptr)
					return nullptr;
				auto error = [&](const auto... message) {
					device->Log()->Error("VulkanBottomLevelAccelerationStructure::Create - ", message...);
					return nullptr;
				};

				if (!device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					return error("Trying to create BLAS on a device with no RT support! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
				VkAccelerationStructureGeometryKHR geometry = {};
				Helpers::FillBasicBuildInfo(device, properties, buildInfo, geometry);
				
				VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
				buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
				device->RT().GetAccelerationStructureBuildSizes(
					*device,
					VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					&buildInfo,
					&properties.maxTriangleCount,
					&buildSizesInfo);

				const Reference<VulkanArrayBuffer> dataBuffer = Object::Instantiate<VulkanArrayBuffer>(
					device, 1u, static_cast<size_t>(buildSizesInfo.accelerationStructureSize), true,
					VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				if (dataBuffer == nullptr)
					return error("Could not allocate memory for the acceleration structure! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VkAccelerationStructureCreateInfoKHR createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
					createInfo.pNext = nullptr;
					createInfo.createFlags = 0; // Do we need capture-replay? Probably not...
					createInfo.buffer = *dataBuffer;
					createInfo.offset = 0u;
					createInfo.size = buildSizesInfo.accelerationStructureSize;
					createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
					createInfo.deviceAddress = 0u;
				}

				VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
				{
					VkResult result = device->RT().CreateAccelerationStructure(*device, &createInfo, device->AllocationCallbacks(), &accelerationStructure);
					if (result != VK_SUCCESS)
						return error("Failed to create acceleration structure! (Error: ", result, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
					assert(accelerationStructure != VK_NULL_HANDLE);
				}

				const Reference<VulkanBottomLevelAccelerationStructure> instance =
					new VulkanBottomLevelAccelerationStructure(accelerationStructure, dataBuffer, buildSizesInfo, properties);
				instance->ReleaseRef();
				return instance;
			}

			VulkanBottomLevelAccelerationStructure::VulkanBottomLevelAccelerationStructure(
				VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
				const VkAccelerationStructureBuildSizesInfoKHR& buildSizes,
				const BottomLevelAccelerationStructure::Properties& properties)
				: VulkanAccelerationStructure(accelerationStructure, buffer, buildSizes)
				, m_properties(properties) {}

			VulkanBottomLevelAccelerationStructure::~VulkanBottomLevelAccelerationStructure() {}

			void VulkanBottomLevelAccelerationStructure::Build(
				CommandBuffer* commandBuffer,
				ArrayBuffer* vertexBuffer, size_t vertexStride, size_t positionFieldOffset,
				ArrayBuffer* indexBuffer,
				BottomLevelAccelerationStructure* updateSrcBlas,
				size_t vertexCount,
				size_t indexCount,
				size_t firstIndex) {
				auto error = [&](const auto... message) {
					Device()->Log()->Error("VulkanBottomLevelAccelerationStructure::Build - ", message...);
				};

				if (vertexStride <= 0u)
					return error("vertexStride must be greater than 0! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const uint32_t indexStride = static_cast<uint32_t>(
					(m_properties.indexFormat == IndexFormat::U16) ? sizeof(uint16_t) : sizeof(uint32_t));

				// Make sure we have vulkan resources:
				VulkanCommandBuffer* const commands = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (commands == nullptr)
					return error("null or incompatible Command Buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VulkanArrayBuffer* const vertices = dynamic_cast<VulkanArrayBuffer*>(vertexBuffer);
				if (vertices == nullptr)
					return error("null or incompatible Vertex buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VulkanArrayBuffer* const indices = dynamic_cast<VulkanArrayBuffer*>(indexBuffer);
				if (indices == nullptr)
					return error("null or incompatible Index buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Fix counts, offsets and sizes:
				{
					const size_t vertexBufferSize = vertices->ObjectSize() * vertices->ObjectCount();
					positionFieldOffset = Math::Min(positionFieldOffset, vertexBufferSize);
					vertexCount = Math::Min((vertexBufferSize - positionFieldOffset) / vertexStride, vertexCount);

					const size_t indexBufferCount = indices->ObjectSize() * indices->ObjectCount() / indexStride;
					firstIndex = Math::Min(firstIndex, indexBufferCount);
					indexCount = Math::Min(indexBufferCount - firstIndex, indexCount);
				}

				// Make sure index count is a multiple of 3:
				{
					const size_t remainder = (indexCount % 3u);
					if (remainder != 0u) {
						Device()->Log()->Warning("VulkanBottomLevelAccelerationStructure::Build - ",
							"Index count not multiple of 3! Discarding indices beyond last valid triangle! ",
							"[File:", __FILE__, "; Line: ", __LINE__, "]");
						indexCount -= remainder;
					}
				}

				// Get source structure and scratch buffer:
				VulkanBottomLevelAccelerationStructure* const srcStructure =
					(m_properties.flags & AccelerationStructure::Flags::ALLOW_UPDATES) != AccelerationStructure::Flags::NONE
					? dynamic_cast<VulkanBottomLevelAccelerationStructure*>(updateSrcBlas) : nullptr;
				const Reference<VulkanArrayBuffer> scratchBuffer = GetScratchBuffer(srcStructure != nullptr);
				if (scratchBuffer == nullptr)
					return error("Could not retrieve the scratch buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");


				// Fill base information:
				VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
				VkAccelerationStructureGeometryKHR geometry = {};
				Helpers::FillBasicBuildInfo(Device(), m_properties, buildInfo, geometry);

				// Provide handles:
				{
					buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
					buildInfo.srcAccelerationStructure = (srcStructure == nullptr) ? VK_NULL_HANDLE : srcStructure->operator VkAccelerationStructureKHR();
					buildInfo.dstAccelerationStructure = (*this);
					buildInfo.scratchData.deviceAddress = scratchBuffer->VulkanDeviceAddress();
				}

				// Provide Vertex & Index buffer information:
				{
					geometry.geometry.triangles.vertexData.deviceAddress = vertices->VulkanDeviceAddress() + positionFieldOffset;
					geometry.geometry.triangles.vertexStride = static_cast<uint32_t>(vertexStride);
					geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(Math::Max(vertexCount, size_t(1u)) - 1u);
					geometry.geometry.triangles.indexData.deviceAddress = indices->VulkanDeviceAddress() + (firstIndex * indexStride);
				}

				// Define range:
				VkAccelerationStructureBuildRangeInfoKHR buildRange = {};
				const VkAccelerationStructureBuildRangeInfoKHR* buildRanges[1u];
				{
					buildRange.primitiveCount = static_cast<uint32_t>(indexCount / 3u);
					buildRange.primitiveOffset = 0u;
					buildRange.firstVertex = 0u;
					buildRange.transformOffset = 0u;
					buildRanges[0u] = &buildRange;
				}

				// Execute build command:
				Device()->RT().CmdBuildAccelerationStructures(*commands, 1u, &buildInfo, buildRanges);

				// Keep references to dependencies:
				if (srcStructure != nullptr)
					commands->RecordBufferDependency(srcStructure);
				if (srcStructure != this)
					commands->RecordBufferDependency(this);
				commands->RecordBufferDependency(vertices);
				commands->RecordBufferDependency(indices);
			}
		}
	}
}
