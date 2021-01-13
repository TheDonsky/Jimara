#include "../../GtestHeaders.h"
#include "Graphics/GraphicsInstance.h"
#include "Graphics/Vulkan/VulkanInstance.h"
#include "Graphics/Vulkan/VulkanPhysicalDevice.h"
#include "Graphics/Vulkan/VulkanLogicalDevice.h"
#include "OS/Logging/StreamLogger.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			TEST(VulkanInstanceTest, CreateInstance) {
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("VulkanInstanceTest", Application::AppVersion(1, 0, 0));
				Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo, GraphicsInstance::Backend::VULKAN);
				EXPECT_NE(Reference<VulkanInstance>(instance), nullptr);
			}

			TEST(VulkanInstanceTest, CreateLogicalDevice) {
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("VulkanInstanceTest", Application::AppVersion(1, 0, 0));
				Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo, GraphicsInstance::Backend::VULKAN);
				EXPECT_NE(Reference<VulkanInstance>(instance), nullptr);
				for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
					Reference<PhysicalDevice> physicalDevice = instance->GetPhysicalDevice(i);
					EXPECT_NE(Reference<VulkanPhysicalDevice>(physicalDevice), nullptr);
					Reference<LogicalDevice> logicalDevice = physicalDevice->CreateLogicalDevice();
					EXPECT_NE(Reference<VulkanLogicalDevice>(logicalDevice), nullptr);
				}
			}
		}
	}
}
