#include "VulkanInstance.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanInstance::VulkanInstance(OS::Logger* logger) : GraphicsInstance(logger) {
			}

			size_t VulkanInstance::PhysicalDeviceCount()const {
				return 0;
			}

			Reference<PhysicalDevice> VulkanInstance::GetPhysicalDevice(size_t index)const {
				return nullptr;
			}
		}
	}
}
