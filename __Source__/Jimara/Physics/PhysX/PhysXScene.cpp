#include "PhysXScene.h"
#include "PhysXStaticBody.h"
#include "PhysXDynamicBody.h"
#include "../../Core/Unused.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				static physx::PxFilterFlags SimulationFilterShader(
					physx::PxFilterObjectAttributes attributes0,
					physx::PxFilterData filterData0,
					physx::PxFilterObjectAttributes attributes1,
					physx::PxFilterData filterData1,
					physx::PxPairFlags& pairFlags,
					const void* constantBlock,
					physx::PxU32 constantBlockSize) {
					Unused(attributes0, attributes1, filterData0, filterData1, constantBlockSize, constantBlock);

					pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT | physx::PxPairFlag::eTRIGGER_DEFAULT
						| physx::PxPairFlag::eSOLVE_CONTACT | physx::PxPairFlag::eDETECT_DISCRETE_CONTACT
						| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
						| physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
						| physx::PxPairFlag::eNOTIFY_TOUCH_LOST
						| physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
					
					return physx::PxFilterFlag::eDEFAULT;
				}
			}

			PhysXScene::PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity) : PhysicsScene(instance) {
				m_dispatcher = physx::PxDefaultCpuDispatcherCreate(static_cast<uint32_t>(max(maxSimulationThreads, static_cast<size_t>(1u))));
				if (m_dispatcher == nullptr) {
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the dispatcher!");
				}
				physx::PxSceneDesc sceneDesc((*instance)->getTolerancesScale());
				sceneDesc.gravity = physx::PxVec3(gravity.x, gravity.y, gravity.z);
				sceneDesc.cpuDispatcher = m_dispatcher;
				sceneDesc.filterShader = SimulationFilterShader;
				sceneDesc.simulationEventCallback = &m_simulationEventCallback;
				m_scene = (*instance)->createScene(sceneDesc);
				if (m_scene == nullptr)
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the scene!");
				physx::PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
				if (pvdClient != nullptr)
				{
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
					pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
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

			Reference<DynamicBody> PhysXScene::AddRigidBody(const Matrix4& pose, bool enabled) {
				return Object::Instantiate<PhysXDynamicBody>(this, pose, enabled);
			}

			Reference<StaticBody> PhysXScene::AddStaticBody(const Matrix4& pose, bool enabled) {
				return Object::Instantiate<PhysXStaticBody>(this, pose, enabled);
			}

			void PhysXScene::SimulateAsynch(float deltaTime) { m_scene->simulate(deltaTime); }

			void PhysXScene::SynchSimulation() { m_scene->fetchResults(true); }

			PhysXScene::operator physx::PxScene* () const { return m_scene; }

			physx::PxScene* PhysXScene::operator->()const { return m_scene; }





			void PhysXScene::SimulationEventCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) { 
				Unused(constraints, count); 
			}
			void PhysXScene::SimulationEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count) { 
				Unused(actors, count); 
			}
			void PhysXScene::SimulationEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count) { 
				Unused(actors, count); 
			}
			void PhysXScene::SimulationEventCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) {
				// __TODO__: Implement this crap!
				Unused(pairHeader, pairs, nbPairs);
			}
			void PhysXScene::SimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) {
				// __TODO__: Implement this crap!
				Unused(pairs, count);
			}
			void PhysXScene::SimulationEventCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) { 
				Unused(bodyBuffer, poseBuffer, count); 
			}
		}
	}
}
