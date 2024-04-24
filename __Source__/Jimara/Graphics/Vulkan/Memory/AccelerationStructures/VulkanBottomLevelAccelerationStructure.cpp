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
				const const BottomLevelAccelerationStructure::Properties& properties)
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
				size_t firstVertex,
				size_t firstIndex) {
				m_buffer->Device()->Log()->Error("VulkanBottomLevelAccelerationStructure::Build - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		}
	}
}
