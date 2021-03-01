#include "../GtestHeaders.h"
#include "OS/Logging/StreamLogger.h"
#include "Components/Transform.h"
#include "Environment/Scene.h"

namespace Jimara {
	namespace {
		Reference<Scene> CreateScene() {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("TransformTest", Application::AppVersion(1, 0, 0));
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo, Graphics::GraphicsInstance::Backend::VULKAN);
			if (graphicsInstance == nullptr) {
				logger->Fatal("TransformTest - CreateScene: Failed to create graphics instance!");
				return nullptr;
			}
			else if (graphicsInstance->PhysicalDeviceCount() > 0) {
				Reference<Graphics::GraphicsDevice> graphicsDevice = graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();
				if (graphicsDevice == nullptr) {
					logger->Fatal("TransformTest - CreateScene: Failed to create graphics device!");
					return nullptr;
				}
				else {
					Reference<AppContext> context = Object::Instantiate<AppContext>(graphicsDevice);
					return Object::Instantiate<Scene>(context);
				}
			}
			else {
				logger->Fatal("TransformTest - CreateScene: No physical device present!");
				return nullptr;
			}
		}
	}


	// Basic tests for local transform fields
	TEST(TransformTest, LocalFields) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		Transform* transform = Object::Instantiate<Transform>(scene->RootObject(), "Transform");

		EXPECT_EQ(transform->LocalPosition(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalPosition(Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalEulerAngles(Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalScale(Vector3(8.0f, 16.0f, 32.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(8.0f, 16.0f, 32.0f));

		transform->Destroy();
	}
}
