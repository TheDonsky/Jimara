#pragma once
#include "VulkanInstance.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Extension functions for Ray-Tracing functions
			/// </summary>
			struct JIMARA_API VulkanRayTracingAPI {
				PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes = nullptr;
				PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructure = nullptr;
				PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddress = nullptr;
				PFN_vkDestroyAccelerationStructureKHR DestroyAccelerationStructure = nullptr;
				PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructures = nullptr;

				/// <summary>
				/// Fills-in the methodss
				/// </summary>
				/// <param name="device"> Logical device handle </param>
				inline void FindAPIMethods(VkDevice device);
			};


			/// <summary>
			/// Fills-in the methodss
			/// </summary>
			/// <param name="device"> Logical device handle </param>
			inline void VulkanRayTracingAPI::FindAPIMethods(VkDevice device) {
				if (device == VK_NULL_HANDLE)
					return;

				auto find = [&](auto& address, const char* name) {
					address = reinterpret_cast<std::remove_reference_t<decltype(address)>>(vkGetDeviceProcAddr(device, name));
				};

				// Acceleration structure:
				{
					find(GetAccelerationStructureBuildSizes, "vkGetAccelerationStructureBuildSizesKHR");
					find(CreateAccelerationStructure, "vkCreateAccelerationStructureKHR");
					find(GetAccelerationStructureDeviceAddress, "vkGetAccelerationStructureDeviceAddressKHR");
					find(DestroyAccelerationStructure, "vkDestroyAccelerationStructureKHR");
					find(CmdBuildAccelerationStructures, "vkCmdBuildAccelerationStructuresKHR");
				}
			}
		}
	}
}
