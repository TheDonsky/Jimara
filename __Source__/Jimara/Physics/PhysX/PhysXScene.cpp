#include "PhysXScene.h"
#include "PhysXRigidBody.h"
#include "PhysXStaticBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXScene::PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity) : PhysicsScene(instance) {
				m_dispatcher = physx::PxDefaultCpuDispatcherCreate(static_cast<uint32_t>(max(maxSimulationThreads, static_cast<size_t>(1u))));
				if (m_dispatcher == nullptr) {
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the dispatcher!");
				}
				physx::PxSceneDesc sceneDesc((*instance)->getTolerancesScale());
				sceneDesc.gravity = physx::PxVec3(gravity.x, gravity.y, gravity.z);
				sceneDesc.cpuDispatcher = m_dispatcher;
				sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
				m_scene = (*instance)->createScene(sceneDesc);
				if (m_scene == nullptr)
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the scene!");
				physx::PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
				if (pvdClient != nullptr)
				{
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
				}
			}

			PhysXScene::~PhysXScene() {
				if (m_scene != nullptr) {
					m_scene->release();
					m_scene = nullptr;
				}
				if (m_dispatcher != nullptr) {
					m_dispatcher->release();
					m_dispatcher = nullptr;
				}
			}
			
			Vector3 PhysXScene::Gravity()const {
				physx::PxVec3 gravity = m_scene->getGravity();
				return Vector3(gravity.x, gravity.y, gravity.z);
			}

			void PhysXScene::SetGravity(const Vector3& value) {
				m_scene->setGravity(physx::PxVec3(value.x, value.y, value.z));
			}

			Reference<RigidBody> PhysXScene::AddRigidBody(const Matrix4& transform, bool enabled) {
				return Object::Instantiate<PhysXRigidBody>(this, transform, enabled);
			}

			Reference<StaticBody> PhysXScene::AddStaticBody(const Matrix4& transform, bool enabled) {
				return Object::Instantiate<PhysXStaticBody>(this, transform, enabled);
			}

			PhysXScene::operator physx::PxScene* () const { return m_scene; }

			physx::PxScene* PhysXScene::operator->()const { return m_scene; }
		}
	}
}
