#include "VulkanAccelerationStructure.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanAccelerationStructure::VulkanAccelerationStructure(
				VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
				const VkAccelerationStructureBuildSizesInfoKHR& buildSizes)
				: m_accelerationStructure(accelerationStructure)
				, m_buffer(buffer)
				, m_buildSizes(buildSizes)
				, m_scratchBufferProvider(TransientBufferSet::Get(buffer->Device()))
				, m_deviceAddress([&]() {
				assert(accelerationStructure != VK_NULL_HANDLE);
				assert(buffer != nullptr);
				VkAccelerationStructureDeviceAddressInfoKHR info = {};
				info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
				info.pNext = nullptr;
				info.accelerationStructure = accelerationStructure;
				return buffer->Device()->RT().GetAccelerationStructureDeviceAddress(*buffer->Device(), &info);
					}()) {
				assert(m_accelerationStructure != VK_NULL_HANDLE);
				assert(m_buffer != nullptr);
				assert((m_buffer->ObjectSize() * m_buffer->ObjectCount()) >= m_buildSizes.accelerationStructureSize);
				assert(m_scratchBufferProvider != nullptr);
			}

			VulkanAccelerationStructure::~VulkanAccelerationStructure() {
				m_buffer->Device()->RT().DestroyAccelerationStructure(
					*m_buffer->Device(), m_accelerationStructure, m_buffer->Device()->AllocationCallbacks());
			}

			VkBuildAccelerationStructureFlagsKHR VulkanAccelerationStructure::GetBuildFlags(AccelerationStructure::Flags flags) {
				auto hasFlag = [&](AccelerationStructure::Flags flag) {
					return (flags & flag) != AccelerationStructure::Flags::NONE;
					};
				return VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR |
					(hasFlag(AccelerationStructure::Flags::ALLOW_UPDATES)
						? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR : 0) |
					(hasFlag(AccelerationStructure::Flags::PREFER_FAST_BUILD)
						? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
						: VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
			}

			VkGeometryFlagsKHR VulkanAccelerationStructure::GetGeometryFlags(AccelerationStructure::Flags flags) {
				auto hasFlag = [&](AccelerationStructure::Flags flag) {
					return (flags & flag) != AccelerationStructure::Flags::NONE;
					};
				return
					(hasFlag(AccelerationStructure::Flags::PREVENT_DUPLICATE_ANY_HIT_INVOCATIONS)
						? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR : 0);
			}
		}
	}
}
