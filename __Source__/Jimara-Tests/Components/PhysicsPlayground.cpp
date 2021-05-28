#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>

#include "Physics/PhysX/PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace {
			
		}

		TEST(PhysicsPlayground, Playground) {
			Jimara::Test::TestEnvironment environment("PhysicsPlayground");
			Reference<PhysicsInstance> instance = PhysicsInstance::Create(environment.RootObject()->Context()->Log());
			ASSERT_NE(instance, nullptr);
			Reference<PhysX::PhysXScene> scene = instance->CreateScene(std::thread::hardware_concurrency() / 4);
			ASSERT_NE(scene, nullptr);
			ASSERT_EQ(scene->Gravity(), PhysicsInstance::DefaultGravity());
			environment.RootObject()->Context()->Log()->Info("Gravity: <", scene->Gravity().x, "; ", scene->Gravity().y, "; ", scene->Gravity().z, ">");
		}
	}
}

