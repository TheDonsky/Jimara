#include "VulkanTopLevelAccelerationStructure.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				static_assert(sizeof(AccelerationStructureInstanceDesc) == sizeof(VkAccelerationStructureInstanceKHR));
				static_assert(offsetof(AccelerationStructureInstanceDesc, transform) == offsetof(VkAccelerationStructureInstanceKHR, transform));
				static_assert(sizeof(AccelerationStructureInstanceDesc::transform) == sizeof(VkAccelerationStructureInstanceKHR::transform));
				static_assert(offsetof(AccelerationStructureInstanceDesc, blasDeviceAddress) == offsetof(VkAccelerationStructureInstanceKHR, accelerationStructureReference));
				static_assert(sizeof(AccelerationStructureInstanceDesc::blasDeviceAddress) == sizeof(VkAccelerationStructureInstanceKHR::accelerationStructureReference));

				static_assert(
					static_cast<uint32_t>(AccelerationStructureInstanceDesc::Flags::DISABLE_BACKFACE_CULLING) ==
					VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR);
				static_assert(
					static_cast<uint32_t>(AccelerationStructureInstanceDesc::Flags::FLIP_FACES) ==
					VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR);
				static_assert(
					static_cast<uint32_t>(AccelerationStructureInstanceDesc::Flags::FORCE_OPAQUE) ==
					VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR);
			}

			struct VulkanTopLevelAccelerationStructure::Helpers {
				inline static void FillBasicBuildInfo(
					const VulkanDevice* device, const TopLevelAccelerationStructure::Properties& properties,
					VkAccelerationStructureBuildGeometryInfoKHR& buildInfo, VkAccelerationStructureGeometryKHR& geometry) {
						{
							buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
							buildInfo.pNext = nullptr;
							buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
							buildInfo.flags = VulkanAccelerationStructure::GetBuildFlags(properties.flags);

							// Ignored during creation:
							buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
							buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
							buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
							buildInfo.scratchData.deviceAddress = 0u;

							buildInfo.geometryCount = 1u;
							buildInfo.pGeometries = &geometry;
							buildInfo.ppGeometries = NULL;
						}

						{
							geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
							geometry.pNext = nullptr;
							geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
							
							geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
							geometry.geometry.instances.pNext = nullptr;
							geometry.geometry.instances.arrayOfPointers = false;
							geometry.geometry.instances.data.deviceAddress = 0u;

							geometry.flags = VulkanAccelerationStructure::GetGeometryFlags(properties.flags);
						}
				}
			};

			Reference<VulkanTopLevelAccelerationStructure> VulkanTopLevelAccelerationStructure::Create(
				VulkanDevice* device, const TopLevelAccelerationStructure::Properties& properties) {
				if (device == nullptr)
					return nullptr;
				auto error = [&](const auto... message) {
					device->Log()->Error("VulkanTopLevelAccelerationStructure::Create - ", message...);
					return nullptr;
				};

				if (!device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					return error("Trying to create TLAS on a device with no RT support! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
				VkAccelerationStructureGeometryKHR geometry = {};
				Helpers::FillBasicBuildInfo(device, properties, buildInfo, geometry);

				VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
				buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
				device->RT().GetAccelerationStructureBuildSizes(
					*device,
					VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					&buildInfo,
					&properties.maxBottomLevelInstances,
					&buildSizesInfo);

				const Reference<VulkanArrayBuffer> dataBuffer = Object::Instantiate<VulkanArrayBuffer>(
					device, 1u, static_cast<size_t>(buildSizesInfo.accelerationStructureSize), true,
					VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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
					createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
					createInfo.deviceAddress = 0u;
				}

				VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
				{
					VkResult result = device->RT().CreateAccelerationStructure(*device, &createInfo, device->AllocationCallbacks(), &accelerationStructure);
					if (result != VK_SUCCESS)
						return error("Failed to create acceleration structure! (Error: ", result, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
					assert(accelerationStructure != VK_NULL_HANDLE);
				}

				const Reference<VulkanTopLevelAccelerationStructure> instance =
					new VulkanTopLevelAccelerationStructure(accelerationStructure, dataBuffer, buildSizesInfo, properties);
				instance->ReleaseRef();
				return instance;
			}

			VulkanTopLevelAccelerationStructure::VulkanTopLevelAccelerationStructure(
				VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
				const VkAccelerationStructureBuildSizesInfoKHR& buildSizes,
				const TopLevelAccelerationStructure::Properties& properties)
				: VulkanAccelerationStructure(accelerationStructure, buffer, buildSizes)
				, m_properties(properties) {}

			VulkanTopLevelAccelerationStructure::~VulkanTopLevelAccelerationStructure() {}

			void VulkanTopLevelAccelerationStructure::Build(
				CommandBuffer* commandBuffer,
				const ArrayBufferReference<AccelerationStructureInstanceDesc>& instanceBuffer,
				TopLevelAccelerationStructure* updateSrcTlas,
				size_t instanceCount,
				size_t firstInstance) {
				auto error = [&](const auto... message) {
					Device()->Log()->Error("VulkanTopLevelAccelerationStructure::Build - ", message...);
				};

				// Make sure we have vulkan resources:
				VulkanCommandBuffer* const commands = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (commands == nullptr)
					return error("null or incompatible Command Buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				VulkanArrayBuffer* const instances = dynamic_cast<VulkanArrayBuffer*>(instanceBuffer.operator Jimara::Graphics::ArrayBuffer *());
				if (instances == nullptr)
					return error("null or incompatible Instance buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Fix counts, offsets and sizes:
				{
					const size_t totalInstanceCount = instances->ObjectCount();
					firstInstance = Math::Min(totalInstanceCount, firstInstance);
					const size_t availableInstanceCount = totalInstanceCount - firstInstance;
					instanceCount = Math::Min(instanceCount, availableInstanceCount);
				}

				// Get source structure and scratch buffer:
				VulkanTopLevelAccelerationStructure* const srcStructure =
					(m_properties.flags & AccelerationStructure::Flags::ALLOW_UPDATES) != AccelerationStructure::Flags::NONE
					? dynamic_cast<VulkanTopLevelAccelerationStructure*>(updateSrcTlas) : nullptr;
				const Reference<VulkanArrayBuffer> scratchBuffer = GetScratchBuffer(srcStructure != nullptr);
				if (scratchBuffer == nullptr)
					return error("Could not retrieve the scratch buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");


				// Fill base information:
				VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
				VkAccelerationStructureGeometryKHR geometry = {};
				Helpers::FillBasicBuildInfo(Device(), m_properties, buildInfo, geometry);

				// Provide handles:
				{
					buildInfo.mode = (srcStructure == nullptr) ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
					buildInfo.srcAccelerationStructure = (srcStructure == nullptr) ? VK_NULL_HANDLE : srcStructure->operator VkAccelerationStructureKHR();
					buildInfo.dstAccelerationStructure = (*this);
					buildInfo.scratchData.deviceAddress = scratchBuffer->VulkanDeviceAddress();
				}

				// Provide Vertex & Index buffer information:
				{
					geometry.geometry.instances.data.deviceAddress = instances->VulkanDeviceAddress();
				}

				// Define range:
				VkAccelerationStructureBuildRangeInfoKHR buildRange = {};
				const VkAccelerationStructureBuildRangeInfoKHR* buildRanges[1u] = {};
				{
					buildRange.primitiveCount = static_cast<uint32_t>(instanceCount);
					buildRange.primitiveOffset = static_cast<uint32_t>(firstInstance * sizeof(AccelerationStructureInstanceDesc));
					buildRange.firstVertex = 0u;
					buildRange.transformOffset = 0u;
					buildRanges[0u] = &buildRange;
				}

				// Barrier before build:
				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				}
				vkCmdPipelineBarrier(*commands,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
					VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
					VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
					0, 1, &barrier, 0, nullptr, 0, nullptr);

				// Execute build command:
				Device()->RT().CmdBuildAccelerationStructures(*commands, 1u, &buildInfo, buildRanges);

				// Keep references to dependencies:
				if (srcStructure != nullptr)
					commands->RecordBufferDependency(srcStructure);
				if (srcStructure != this)
					commands->RecordBufferDependency(this);
				commands->RecordBufferDependency(instances);
			}
		}
	}
}
