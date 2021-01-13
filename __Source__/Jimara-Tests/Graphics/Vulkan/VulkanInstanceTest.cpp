#include "../../GtestHeaders.h"
#include "Graphics/GraphicsInstance.h"
#include "Graphics/Vulkan/VulkanInstance.h"
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
		}
	}
}
