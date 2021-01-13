#pragma once
#include "VulkanAPIIncludes.h"
#include "../GraphicsInstance.h"
#include <vector>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanInstance : public GraphicsInstance {
			public:
				VulkanInstance(OS::Logger* logger);

				virtual size_t PhysicalDeviceCount()const override;

				virtual Reference<PhysicalDevice> GetPhysicalDevice(size_t index)const override;

			private:
				VkInstance m_instance;
#ifndef NDEBUG
				std::vector<const char*> m_validationLayers;
				VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

			};
		}
	}
}
