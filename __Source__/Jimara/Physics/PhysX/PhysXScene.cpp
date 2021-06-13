#include "PhysXScene.h"
#include "PhysXStaticBody.h"
#include "PhysXDynamicBody.h"
#include "../../Core/Unused.h"


#pragma warning(disable: 26812)
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
					Unused(filterData0, filterData1, constantBlockSize, constantBlock);

					pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT | physx::PxPairFlag::eTRIGGER_DEFAULT
						| physx::PxPairFlag::eSOLVE_CONTACT | physx::PxPairFlag::eDETECT_DISCRETE_CONTACT
						| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
						| physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
						| physx::PxPairFlag::eNOTIFY_TOUCH_LOST
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST
						| physx::PxPairFlag::eNOTIFY_CONTACT_POINTS
						| physx::PxPairFlag::eNOTIFY_TOUCH_CCD;

					if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
						pairFlags &= ~(physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS);
					
					return physx::PxFilterFlag::eDEFAULT;
				}
			}

			PhysXScene::PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity) : PhysicsScene(instance) {
				m_dispatcher = physx::PxDefaultCpuDispatcherCreate(static_cast<uint32_t>(max(maxSimulationThreads, static_cast<size_t>(1u))));
				if (m_dispatcher == nullptr) {
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the dispatcher!");
					return;
				}
				physx::PxSceneDesc sceneDesc((*instance)->getTolerancesScale());
				sceneDesc.gravity = physx::PxVec3(gravity.x, gravity.y, gravity.z);
				sceneDesc.cpuDispatcher = m_dispatcher;
				sceneDesc.filterShader = SimulationFilterShader;
				sceneDesc.simulationEventCallback = &m_simulationEventCallback;
				sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
				sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
				sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
				m_scene = (*instance)->createScene(sceneDesc);
				if (m_scene == nullptr) {
					APIInstance()->Log()->Fatal("PhysicXScene - Failed to create the scene!");
					return;
				}
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

			void PhysXScene::SynchSimulation() { 
				m_scene->fetchResults(true);
				m_simulationEventCallback.NotifyEvents();
			}

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
				Unused(pairHeader);
				std::unique_lock<std::mutex> lock(m_eventLock);
				size_t bufferId = m_backBuffer;
				std::vector<PhysicsCollider::ContactPoint>& pointBuffer = m_contactPoints[bufferId];
				for (size_t i = 0; i < nbPairs; i++) {
					const physx::PxContactPair& pair = pairs[i];

					ContactPairInfo info = {};
					info.info.type =
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) != 0) ? PhysicsCollider::ContactType::ON_COLLISION_BEGIN :
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST) != 0) ? PhysicsCollider::ContactType::ON_COLLISION_END :
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS) != 0) ? PhysicsCollider::ContactType::ON_COLLISION_PERSISTS :
						PhysicsCollider::ContactType::CONTACT_TYPE_COUNT;
					if (info.info.type >= PhysicsCollider::ContactType::CONTACT_TYPE_COUNT) continue;

					if (pair.shapes[0] < pair.shapes[1]) {
						info.shapes[0] = pair.shapes[0];
						info.shapes[1] = pair.shapes[1];
						info.info.reverseOrder = false;
					}
					else {
						info.shapes[0] = pair.shapes[1];
						info.shapes[1] = pair.shapes[0];
						info.info.reverseOrder = true;
					}
					if (info.shapes[0]->userData == nullptr || info.shapes[1]->userData == nullptr) continue;

					if (m_contactPointBuffer.size() < pair.contactCount) m_contactPointBuffer.resize(pair.contactCount);
					size_t contactCount = pair.extractContacts(m_contactPointBuffer.data(), (uint32_t)m_contactPointBuffer.size());

					info.info.pointBuffer = bufferId;
					info.info.firstContactPoint = pointBuffer.size();
					for (size_t i = 0; i < m_contactPointBuffer.size(); i++) {
						const physx::PxContactPairPoint& point = m_contactPointBuffer[i];
						PhysicsCollider::ContactPoint info = {};
						info.position = Translate(point.position);
						info.normal = Translate(point.normal);
						pointBuffer.push_back(info);
					}
					info.info.lastContactPoint = pointBuffer.size();

					m_contacts.push_back(info);
				}
			}
			void PhysXScene::SimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) {
				std::unique_lock<std::mutex> lock(m_eventLock);
				size_t bufferId = m_backBuffer;
				for (size_t i = 0; i < count; i++) {
					const physx::PxTriggerPair& pair = pairs[i];
					ContactPairInfo info = {};
					info.info.type =
						(pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) ? PhysicsCollider::ContactType::ON_TRIGGER_BEGIN :
						(pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST) ? PhysicsCollider::ContactType::ON_TRIGGER_END :
						PhysicsCollider::ContactType::CONTACT_TYPE_COUNT;
					if (info.info.type >= PhysicsCollider::ContactType::CONTACT_TYPE_COUNT) continue;

					if (pair.triggerShape < pair.otherShape) {
						info.shapes[0] = pair.triggerShape;
						info.shapes[1] = pair.otherShape;
						info.info.reverseOrder = false;
					}
					else {
						info.shapes[0] = pair.otherShape;
						info.shapes[1] = pair.triggerShape;
						info.info.reverseOrder = true;
					}
					if (info.shapes[0]->userData == nullptr || info.shapes[1]->userData == nullptr) continue;
					m_contacts.push_back(info);
				}
			}
			void PhysXScene::SimulationEventCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) { 
				Unused(bodyBuffer, poseBuffer, count); 
			}

			void PhysXScene::SimulationEventCallback::NotifyEvents() {
				std::unique_lock<std::mutex> lock(m_eventLock);
				
				// Current contact point buffer:
				const size_t bufferId = m_backBuffer;
				std::vector<PhysicsCollider::ContactPoint>& pointBuffer = m_contactPoints[bufferId];
				
				// Notifies listeners about the pair contact (returns false, if the shapes are no longer valid):
				auto notifyContact = [&](const ShapePair& pair, ContactInfo& info) {
					ContactEventListener* listener = (ContactEventListener*)pair.shapes[0]->userData;
					ContactEventListener* otherListener = (ContactEventListener*)pair.shapes[1]->userData;
					if (listener == nullptr || otherListener == nullptr) return false;
					PhysicsCollider::ContactPoint* const contactPoints = pointBuffer.data() + info.firstContactPoint;
					const size_t contactPointCount = (info.lastContactPoint - info.firstContactPoint);
					auto reverse = [&]() {
						for (size_t i = 0; i < contactPointCount; i++) {
							PhysicsCollider::ContactPoint& point = contactPoints[i];
							point.normal = -point.normal;
						}
						info.reverseOrder ^= 1;
					};
					if (info.reverseOrder) {
						otherListener->OnContact(pair.shapes[1], pair.shapes[0], info.type, contactPoints, contactPointCount);
						reverse();
						listener->OnContact(pair.shapes[0], pair.shapes[1], info.type, contactPoints, contactPointCount);
					}
					else {
						listener->OnContact(pair.shapes[0], pair.shapes[1], info.type, contactPoints, contactPointCount);
						reverse();
						otherListener->OnContact(pair.shapes[1], pair.shapes[0], info.type, contactPoints, contactPointCount);
					}
					return true;
				};

				// Notifies about the newly contacts and saves persistent contacts in case the actors start sleeping:
				for (size_t contactId = 0; contactId < m_contacts.size(); contactId++) {
					ContactPairInfo& info = m_contacts[contactId];
					ShapePair pair;
					pair.shapes[0] = info.shapes[0];
					pair.shapes[1] = info.shapes[1];
					notifyContact(pair, info.info);
					if (info.info.type == PhysicsCollider::ContactType::ON_COLLISION_END || 
						info.info.type == PhysicsCollider::ContactType::ON_TRIGGER_END)
						m_persistentContacts.erase(pair);
					else {
						ContactInfo contact = info.info;
						if (contact.type == PhysicsCollider::ContactType::ON_COLLISION_BEGIN)
							contact.type = PhysicsCollider::ContactType::ON_COLLISION_PERSISTS;
						else if (contact.type == PhysicsCollider::ContactType::ON_TRIGGER_BEGIN)
							contact.type = PhysicsCollider::ContactType::ON_TRIGGER_PERSISTS;
						m_persistentContacts[pair] = contact;
					}
				}

				// Notifies about sleeping persistent contacts:
				for (PersistentContactMap::iterator it = m_persistentContacts.begin(); it != m_persistentContacts.end(); ++it) {
					ContactInfo& info = it->second;
					if (info.pointBuffer == bufferId) continue;
					const size_t contactPointCount = (info.lastContactPoint - info.firstContactPoint);
					const PhysicsCollider::ContactPoint* const contactPoints = m_contactPoints[info.pointBuffer].data() + info.firstContactPoint;
					info.firstContactPoint = pointBuffer.size();
					for (size_t i = 0; i < contactPointCount; i++)
						pointBuffer.push_back(contactPoints[i]);
					info.lastContactPoint = pointBuffer.size();
					info.pointBuffer = bufferId;
					if (!notifyContact(it->first, info)) m_pairsToRemove.push_back(it->first);
				}

				// Remove invalidated persistent contacts:
				for (size_t i = 0; i < m_pairsToRemove.size(); i++)
					m_persistentContacts.erase(m_pairsToRemove[i]);
				m_pairsToRemove.clear();

				// Swaps contact buffers:
				m_backBuffer ^= 1;
				m_contacts.clear();
				m_contactPoints[m_backBuffer].clear();
			}
		}
	}
}
#pragma warning(default: 26812)
