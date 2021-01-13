#include "../../GtestHeaders.h"
#include "Graphics/GraphicsInstance.h"
#include "Graphics/Vulkan/VulkanInstance.h"
#include "Graphics/Vulkan/VulkanPhysicalDevice.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "OS/Logging/StreamLogger.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			// Makes sure Vulkan device can be instantiated properly
			TEST(VulkanInstanceTest, CreateInstance) {
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("VulkanInstanceTest", Application::AppVersion(1, 0, 0));
				Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo, GraphicsInstance::Backend::VULKAN);
				EXPECT_NE(Reference<VulkanInstance>(instance), nullptr);
			}

			// Makes sure all logical devices can produce physical devices
			TEST(VulkanInstanceTest, CreateLogicalDevice) {
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("VulkanInstanceTest", Application::AppVersion(1, 0, 0));
				Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo, GraphicsInstance::Backend::VULKAN);
				EXPECT_NE(Reference<VulkanInstance>(instance), nullptr);
				for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
					Reference<PhysicalDevice> physicalDevice = instance->GetPhysicalDevice(i);
					EXPECT_NE(Reference<VulkanPhysicalDevice>(physicalDevice), nullptr);
					Reference<GraphicsDevice> logicalDevice = physicalDevice->CreateLogicalDevice();
					EXPECT_NE(Reference<VulkanDevice>(logicalDevice), nullptr);
				}
			}
		}
	}
}
