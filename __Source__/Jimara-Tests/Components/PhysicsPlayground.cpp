#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>

#include "Physics/PhysX/PhysXInstance.h"


namespace Jimara {
	namespace Physics {
		namespace {
			class PhysicXScene : public virtual Object {
			private:
				const Reference<PhysX::PhysXInstance> m_instance;
				physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
				physx::PxScene* m_scene = nullptr;

			public:
				inline PhysicXScene(PhysX::PhysXInstance* instance) : m_instance(instance) {
					m_dispatcher = physx::PxDefaultCpuDispatcherCreate(max(std::thread::hardware_concurrency() / 4u, 1u));
					if (m_dispatcher == nullptr) {
						m_instance->Log()->Fatal("PhysicXScene - Failed to create the dispatcher!");
					}

					physx::PxSceneDesc sceneDesc((*m_instance)->getTolerancesScale());
					sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
					sceneDesc.cpuDispatcher = m_dispatcher;
					sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
					m_scene = (*m_instance)->createScene(sceneDesc);
					if (m_scene == nullptr)
						m_instance->Log()->Fatal("PhysicXScene - Failed to create the scene!");
				}

				inline virtual ~PhysicXScene() {
					if (m_scene != nullptr) {
						m_scene->release();
						m_scene = nullptr;
					}
					if (m_dispatcher != nullptr) {
						m_dispatcher->release();
						m_dispatcher = nullptr;
					}
				}

				physx::PxScene* Scene()const { return m_scene; }
			};
		}

		TEST(PhysicsPlayground, Playground) {
			Jimara::Test::TestEnvironment environment("PhysicsPlayground");
			Reference<PhysX::PhysXInstance> instance = PhysicsInstance::Create(environment.RootObject()->Context()->Log());
			ASSERT_NE(instance, nullptr);
			Reference<PhysicXScene> scene = Object::Instantiate<PhysicXScene>(instance);
		}
	}
}

