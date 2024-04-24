#include "VulkanAccelerationStructure.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanAccelerationStructure::VulkanAccelerationStructure(
				VkAccelerationStructureKHR accelerationStructure, VulkanArrayBuffer* buffer,
				const VkAccelerationStructureBuildSizesInfoKHR& buildSizes)
				: m_accelerationStructure(accelerationStructure)
				, m_buffer(buffer)
				, m_buildSizes(buildSizes) {
				assert(m_accelerationStructure != VK_NULL_HANDLE);
				assert(m_buffer != nullptr);
				assert((m_buffer->ObjectSize() * m_buffer->ObjectCount()) >= m_buildSizes.accelerationStructureSize);
			}

			VulkanAccelerationStructure::~VulkanAccelerationStructure() {
				m_buffer->Device()->RT().DestroyAccelerationStructure(
					*m_buffer->Device(), m_accelerationStructure, m_buffer->Device()->AllocationCallbacks());
			}

			inline VkBuildAccelerationStructureFlagsKHR VulkanAccelerationStructure::GetBuildFlags(AccelerationStructure::Flags flags) {
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

			inline VkGeometryFlagsKHR VulkanAccelerationStructure::GetGeometryFlags(AccelerationStructure::Flags flags) {
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
