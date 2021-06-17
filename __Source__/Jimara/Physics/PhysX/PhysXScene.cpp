#include "PhysXScene.h"
#include "PhysXStaticBody.h"
#include "PhysXDynamicBody.h"
#include "../../Core/Unused.h"
#include "PhysXCollider.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
#define JIMARA_PHYSX_LAYER_COUNT 256
#define JIMARA_PHYSX_LAYER_DATA_BYTE_ID(layerId)  (layerId >> 3)

#define JIMARA_PHYSX_LAYER_DATA_WIDTH JIMARA_PHYSX_LAYER_DATA_BYTE_ID(JIMARA_PHYSX_LAYER_COUNT)
#define JIMARA_PHYSX_LAYER_FILTER_DATA_SIZE (JIMARA_PHYSX_LAYER_COUNT * JIMARA_PHYSX_LAYER_DATA_WIDTH)

#define JIMARA_PHYSX_GET_LAYER_DATA_BYTE(data, layerA, layerB) data[(layerA * JIMARA_PHYSX_LAYER_DATA_WIDTH) + JIMARA_PHYSX_LAYER_DATA_BYTE_ID(layerB)]
#define JIMARA_PHYSX_LAYER_DATA_BIT(layerB) static_cast<uint8_t>(1u << (layerB & 7))
#define JIMARA_PHYSX_GET_LAYER_DATA_BIT(data, layerA, layerB) ((JIMARA_PHYSX_GET_LAYER_DATA_BYTE(data, layerA, layerB) & JIMARA_PHYSX_LAYER_DATA_BIT(layerB)) != 0)

				static PX_INLINE physx::PxFilterFlags SimulationFilterShader(
					physx::PxFilterObjectAttributes attributes0,
					physx::PxFilterData filterData0,
					physx::PxFilterObjectAttributes attributes1,
					physx::PxFilterData filterData1,
					physx::PxPairFlags& pairFlags,
					const void* constantBlock,
					physx::PxU32 constantBlockSize) {
					Unused(attributes0, attributes1, constantBlockSize, constantBlock);

					PhysicsCollider::BitId layerA = PhysXCollider::GetLayer(filterData0);
					PhysicsCollider::BitId layerB = PhysXCollider::GetLayer(filterData1);
					if (!JIMARA_PHYSX_GET_LAYER_DATA_BIT(static_cast<const uint8_t*>(constantBlock), layerA, layerB))
						return physx::PxFilterFlag::eSUPPRESS;

					if ((PhysXCollider::GetFilterFlags(filterData0) & static_cast<PhysXCollider::FilterFlags>(PhysXCollider::FilterFlag::IS_TRIGGER)) != 0 ||
						(PhysXCollider::GetFilterFlags(filterData1) & static_cast<PhysXCollider::FilterFlags>(PhysXCollider::FilterFlag::IS_TRIGGER)) != 0) {
						pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
					}
					else pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

					pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_CCD
						| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
						| physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
						| physx::PxPairFlag::eNOTIFY_TOUCH_LOST
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
						| physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST;
					
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
				m_layerFilterData = new uint8_t[JIMARA_PHYSX_LAYER_FILTER_DATA_SIZE];
				for (size_t i = 0; i < JIMARA_PHYSX_LAYER_FILTER_DATA_SIZE; i++) m_layerFilterData[i] = ~((uint8_t)0);
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
				if (m_layerFilterData != nullptr) {
					delete[] m_layerFilterData;
					m_layerFilterData = nullptr;
				}
			}
			
			Vector3 PhysXScene::Gravity()const {
				physx::PxVec3 gravity = m_scene->getGravity();
				return Vector3(gravity.x, gravity.y, gravity.z);
			}

			void PhysXScene::SetGravity(const Vector3& value) {
				m_scene->setGravity(physx::PxVec3(value.x, value.y, value.z));
			}

			bool PhysXScene::LayersInteract(PhysicsCollider::BitId a, PhysicsCollider::BitId b)const {
				if (m_layerFilterData == nullptr) return false;
				else return JIMARA_PHYSX_GET_LAYER_DATA_BIT(m_layerFilterData, a, b);
			}

			void PhysXScene::FilterLayerInteraction(PhysicsCollider::BitId a, PhysicsCollider::BitId b, bool enableIntaraction) {
				if (m_layerFilterData == nullptr) {
					APIInstance()->Log()->Fatal("PhysXScene::FilterLayerInteraction - layer filter data missing!");
					return;
				}
				if (enableIntaraction) {
					JIMARA_PHYSX_GET_LAYER_DATA_BYTE(m_layerFilterData, a, b) |= JIMARA_PHYSX_LAYER_DATA_BIT(b);
					JIMARA_PHYSX_GET_LAYER_DATA_BYTE(m_layerFilterData, b, a) |= JIMARA_PHYSX_LAYER_DATA_BIT(a);
				}
				else {
					JIMARA_PHYSX_GET_LAYER_DATA_BYTE(m_layerFilterData, a, b) &= ~JIMARA_PHYSX_LAYER_DATA_BIT(b);
					JIMARA_PHYSX_GET_LAYER_DATA_BYTE(m_layerFilterData, b, a) &= ~JIMARA_PHYSX_LAYER_DATA_BIT(a);
				}
				m_layerFilterDataDirty = true;
			}

			Reference<DynamicBody> PhysXScene::AddRigidBody(const Matrix4& pose, bool enabled) {
				return Object::Instantiate<PhysXDynamicBody>(this, pose, enabled);
			}

			Reference<StaticBody> PhysXScene::AddStaticBody(const Matrix4& pose, bool enabled) {
				return Object::Instantiate<PhysXStaticBody>(this, pose, enabled);
			}

			namespace {
				inline static RaycastHit TranslateHit(const physx::PxLocationHit& hitInfo) {
					RaycastHit hit;
					hit.collider = ((PhysXCollider::UserData*)hitInfo.shape->userData)->Collider();
					hit.normal = Translate(hitInfo.normal);
					hit.point = Translate(hitInfo.position);
					hit.distance = hitInfo.distance;
					return hit;
				};

				struct QueryFilterCallback : public physx::PxQueryFilterCallback {
					const PhysicsCollider::LayerMask layers;
					const Function<PhysicsScene::QueryFilterFlag, PhysicsCollider*>* const preFilterCallback = nullptr;
					const Function<PhysicsScene::QueryFilterFlag, const RaycastHit&>* const postFilterCallback = nullptr;
					const bool findAll = false;
					const physx::PxQueryFilterData filterData;
					
					inline physx::PxQueryHitType::Enum TypeFromFlag(PhysicsScene::QueryFilterFlag flag) {
						return
							(flag == PhysicsScene::QueryFilterFlag::REPORT) ? (findAll ? physx::PxQueryHitType::eTOUCH : physx::PxQueryHitType::eBLOCK) :
							(flag == PhysicsScene::QueryFilterFlag::REPORT_BLOCK) ? physx::PxQueryHitType::eBLOCK : physx::PxQueryHitType::eNONE;
					}

					inline virtual physx::PxQueryHitType::Enum preFilter(
						const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override {
						Unused(filterData, actor, queryFlags);
						PhysXCollider::UserData* data = (PhysXCollider::UserData*)shape->userData;
						if (data == nullptr) return physx::PxQueryHitType::eNONE;
						PhysicsCollider* collider = data->Collider();
						if (collider == nullptr) return physx::PxQueryHitType::eNONE;
						else if (!layers[collider->GetLayer()]) return physx::PxQueryHitType::eNONE;
						else if (preFilterCallback != nullptr) return TypeFromFlag((*preFilterCallback)(collider));
						else return (findAll ? physx::PxQueryHitType::eTOUCH : physx::PxQueryHitType::eBLOCK);
					}

					inline virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override {
						Unused(filterData);
						if (hit.shape->userData == nullptr) return physx::PxQueryHitType::eNONE;
						RaycastHit checkHit = TranslateHit((physx::PxLocationHit&)hit);
						if (checkHit.collider == nullptr) return physx::PxQueryHitType::eNONE;
						else return TypeFromFlag((*postFilterCallback)(checkHit));
					}

					inline QueryFilterCallback(const PhysicsCollider::LayerMask& mask
						, const Function<PhysicsScene::QueryFilterFlag, PhysicsCollider*>* preFilterCall
						, const Function<PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilterCall
						, bool reportAll)
						: layers(mask)
						, preFilterCallback(preFilterCall), postFilterCallback(postFilterCall)
						, findAll(reportAll)
						, filterData([&]() {
						physx::PxQueryFilterData data;
						data.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::ePREFILTER;
						if (postFilterCall != nullptr) data.flags |= physx::PxQueryFlag::ePOSTFILTER;
						if (reportAll && preFilterCall == nullptr && postFilterCall == nullptr) data.flags |= physx::PxQueryFlag::eNO_BLOCK;
						return data;
							}()) {}
				};

				template<typename HitType>
				class MultiHitCallbacks : public virtual physx::PxHitCallback<HitType> {
				private:
					HitType m_touchBuffer[128];
					const Callback<const RaycastHit&>* m_onHitFound;
					size_t m_numTouches = 0;

				public:
					inline MultiHitCallbacks(const Callback<const RaycastHit&>* onHitFound) 
						: physx::PxHitCallback<HitType>(m_touchBuffer, static_cast<physx::PxU32>(sizeof(m_touchBuffer) / sizeof(HitType)))
						, m_onHitFound(onHitFound) { }

					inline virtual physx::PxAgain processTouches(const HitType* buffer, physx::PxU32 nbHits) override {
						for (physx::PxU32 i = 0; i < nbHits; i++) (*m_onHitFound)(TranslateHit(buffer[i]));
						m_numTouches += nbHits;
						return true;
					}

					inline size_t NumTouches()const { return m_numTouches; }
				};
			}

			size_t PhysXScene::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
				, const Callback<const RaycastHit&>& onHitFound
				, const PhysicsCollider::LayerMask& layerMask, bool reportAll
				, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter, const Function<QueryFilterFlag, const RaycastHit&>* postFilter)const {
				static_assert(sizeof(physx::PxFilterData) >= sizeof(PhysicsCollider::LayerMask*));
				physx::PxVec3 dir;
				{
					if (maxDistance < 0.0f) {
						maxDistance = -maxDistance;
						dir = -Translate(direction);
					}
					else dir = Translate(direction);
					float rawDirMagn = dir.magnitude();
					if (rawDirMagn <= 0.0f) return 0;
					else dir /= rawDirMagn;
				}
				QueryFilterCallback filterCallback(layerMask, preFilter, postFilter, reportAll);
				physx::PxHitFlags hitFlags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL;
				if (reportAll) {
					MultiHitCallbacks<physx::PxRaycastHit> hitBuff(&onHitFound);
					m_scene->raycast(Translate(origin), dir, maxDistance, hitBuff, hitFlags | physx::PxHitFlag::eMESH_MULTIPLE, filterCallback.filterData, &filterCallback);
					if (hitBuff.hasBlock) {
						onHitFound(TranslateHit(hitBuff.block));
						return hitBuff.NumTouches() + 1;
					} 
					else return hitBuff.NumTouches();
				}
				else {
					physx::PxRaycastBuffer hitBuff;
					if (m_scene->raycast(Translate(origin), dir, maxDistance, hitBuff, hitFlags, filterCallback.filterData, &filterCallback)) {
						assert(hitBuff.hasBlock);
						onHitFound(TranslateHit(hitBuff.block));
						return 1;
					}
					else return 0;
				}
			}

			void PhysXScene::SimulateAsynch(float deltaTime) { 
				if (m_layerFilterDataDirty) {
					m_scene->setFilterShaderData(m_layerFilterData, static_cast<physx::PxU32>(JIMARA_PHYSX_LAYER_FILTER_DATA_SIZE));
					m_layerFilterDataDirty = false;
				}
				m_scene->simulate(deltaTime); 
			}

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
				uint8_t bufferId = m_backBuffer;
				std::vector<PhysicsCollider::ContactPoint>& pointBuffer = m_contactPoints[bufferId];
				for (size_t i = 0; i < nbPairs; i++) {
					const physx::PxContactPair& pair = pairs[i];

					PhysXCollider::UserData* data[2] = { (PhysXCollider::UserData*)pair.shapes[0]->userData, (PhysXCollider::UserData*)pair.shapes[1]->userData };
					if (data[0] == nullptr || data[1] == nullptr) continue;
					bool isTriggerContact = data[0]->Collider()->IsTrigger() || data[1]->Collider()->IsTrigger();

					ContactPairInfo info = {};
					info.info.type =
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) != 0) 
						? (isTriggerContact ? PhysicsCollider::ContactType::ON_TRIGGER_BEGIN : PhysicsCollider::ContactType::ON_COLLISION_BEGIN) :
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST) != 0) 
						? (isTriggerContact ? PhysicsCollider::ContactType::ON_TRIGGER_END : PhysicsCollider::ContactType::ON_COLLISION_END) :
						(((physx::PxU16)pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS) != 0) 
						? (isTriggerContact ? PhysicsCollider::ContactType::ON_TRIGGER_PERSISTS : PhysicsCollider::ContactType::ON_COLLISION_PERSISTS) :
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

					if (m_contactPointBuffer.size() < pair.contactCount) m_contactPointBuffer.resize(pair.contactCount);
					size_t contactCount = pair.extractContacts(m_contactPointBuffer.data(), (uint32_t)m_contactPointBuffer.size());

					info.info.pointBuffer = bufferId;
					info.info.firstContactPoint = pointBuffer.size();
					for (size_t i = 0; i < contactCount; i++) {
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
				uint8_t bufferId = m_backBuffer;
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
					info.info.pointBuffer = bufferId;
					m_contacts.push_back(info);
				}
			}
			void PhysXScene::SimulationEventCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) { 
				Unused(bodyBuffer, poseBuffer, count); 
			}

			void PhysXScene::SimulationEventCallback::NotifyEvents() {
				std::unique_lock<std::mutex> lock(m_eventLock);
				
				// Current contact point buffer:
				const uint8_t bufferId = m_backBuffer;
				std::vector<PhysicsCollider::ContactPoint>& pointBuffer = m_contactPoints[bufferId];
				
				// Notifies listeners about the pair contact (returns false, if the shapes are no longer valid):
				auto notifyContact = [&](const ShapePair& pair, ContactInfo& info) {
					PhysXCollider::UserData* listener = (PhysXCollider::UserData*)pair.shapes[0]->userData;
					PhysXCollider::UserData* otherListener = (PhysXCollider::UserData*)pair.shapes[1]->userData;
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
