#pragma once
#include "PhysXInstance.h"
#include "../../Math/Helpers.h"
#include <unordered_map>


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// Wrapper on top of physx::PxScene
			/// </summary>
			class PhysXScene : public virtual PhysicsScene {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="instance"> "Owner" PhysicsInstance </param>
				/// <param name="maxSimulationThreads"> Maximal number of threads the simulation can use </param>
				/// <param name="gravity"> Gravity </param>
				/// <param name="useScratchBuffer"> Toggles whether or not the scene uses scratch buffer </param>
				PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity, bool useScratchBuffer);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXScene();

				/// <summary> Scene gravity </summary>
				virtual Vector3 Gravity()const override;

				/// <summary>
				/// Tells, if two collider layers interact
				/// </summary>
				/// <param name="a"> First layer </param>
				/// <param name="b"> Second layer </param>
				/// <returns> True, if the colliders from given layers interact </returns>
				virtual bool LayersInteract(PhysicsCollider::Layer a, PhysicsCollider::Layer b)const override;

				/// <summary>
				/// Marks, whether or not the colliders on given layers should interact
				/// </summary>
				/// <param name="a"> First layer </param>
				/// <param name="b"> Second layer </param>
				/// <param name="enableIntaraction"> True, if the colliders from given layers should interact </param>
				virtual void FilterLayerInteraction(PhysicsCollider::Layer a, PhysicsCollider::Layer b, bool enableIntaraction) override;

				/// <summary>
				/// Sets scene gravity
				/// </summary>
				/// <param name="value"> Gravity to use for the scene </param>
				virtual void SetGravity(const Vector3& value) override;

				/// <summary>
				/// Creates a dynimic body (ei rigidbody)
				/// </summary>
				/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="enabled"> If true, the body we create will start off as enabled </param>
				/// <returns> New dynamic body </returns>
				virtual Reference<DynamicBody> AddRigidBody(const Matrix4& pose, bool enabled) override;

				/// <summary>
				/// Creates a static body (ei regular immobile obstacle)
				/// </summary>
				/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="enabled"> If true, the body we create will start off as enabled </param>
				/// <returns> New static body </returns>
				virtual Reference<StaticBody> AddStaticBody(const Matrix4& pose, bool enabled) override;

				/// <summary>
				/// Casts a ray into the scene and reports what it manages to hit
				/// </summary>
				/// <param name="origin"> Ray origin </param>
				/// <param name="direction"> Ray direction </param>
				/// <param name="maxDistance"> Max distance, the ray is allowed to travel </param>
				/// <param name="onHitFound"> If the ray hits something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
				/// <returns> Number of reported RaycastHit-s </returns>
				virtual size_t Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
					, const Callback<const RaycastHit&>& onHitFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter = nullptr, const Function<QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const override;

				/// <summary>
				/// Sweeps a shape through the scene and reports what it manages to hit
				/// </summary>
				/// <param name="shape"> 'Object' to sweep </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="direction"> Sweep direction </param>
				/// <param name="maxDistance"> Max distance, the shape is allowed to travel </param>
				/// <param name="onHitFound"> If the query hits something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
				/// <returns> Number of reported RaycastHit-s </returns>
				virtual size_t Sweep(const SphereShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
					, const Callback<const RaycastHit&>& onHitFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter = nullptr, const Function<QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const override;

				/// <summary>
				/// Sweeps a shape through the scene and reports what it manages to hit
				/// </summary>
				/// <param name="shape"> 'Object' to sweep </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="direction"> Sweep direction </param>
				/// <param name="maxDistance"> Max distance, the shape is allowed to travel </param>
				/// <param name="onHitFound"> If the query hits something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
				/// <returns> Number of reported RaycastHit-s </returns>
				virtual size_t Sweep(const CapsuleShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
					, const Callback<const RaycastHit&>& onHitFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter = nullptr, const Function<QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const override;

				/// <summary>
				/// Sweeps a shape through the scene and reports what it manages to hit
				/// </summary>
				/// <param name="shape"> 'Object' to sweep </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="direction"> Sweep direction </param>
				/// <param name="maxDistance"> Max distance, the shape is allowed to travel </param>
				/// <param name="onHitFound"> If the query hits something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
				/// <returns> Number of reported RaycastHit-s </returns>
				virtual size_t Sweep(const BoxShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
					, const Callback<const RaycastHit&>& onHitFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter = nullptr, const Function<QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const override;


				/// <summary>
				/// Checks if any of the active colliders overlaps with the shape
				/// </summary>
				/// <param name="shape"> 'Object' to check against </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="onOverlapFound"> If the query finds something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="filter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <returns> Number of reported colliders </returns>
				virtual size_t Overlap(const SphereShape& shape, const Matrix4& pose
					, const Callback<PhysicsCollider*>& onOverlapFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* filter = nullptr)const override;

				/// <summary>
				/// Checks if any of the active colliders overlaps with the shape
				/// </summary>
				/// <param name="shape"> 'Object' to check against </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="onOverlapFound"> If the query finds something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="filter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <returns> Number of reported colliders </returns>
				virtual size_t Overlap(const CapsuleShape& shape, const Matrix4& pose
					, const Callback<PhysicsCollider*>& onOverlapFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* filter = nullptr)const override;

				/// <summary>
				/// Checks if any of the active colliders overlaps with the shape
				/// </summary>
				/// <param name="shape"> 'Object' to check against </param>
				/// <param name="pose"> Pose matrix for the shape (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="onOverlapFound"> If the query finds something, this callback will be invoked with the hit info </param>
				/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
				/// <param name="flags"> Query flags for high level query options </param>
				/// <param name="filter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
				/// <returns> Number of reported colliders </returns>
				virtual size_t Overlap(const BoxShape& shape, const Matrix4& pose
					, const Callback<PhysicsCollider*>& onOverlapFound
					, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), QueryFlags flags = 0
					, const Function<QueryFilterFlag, PhysicsCollider*>* filter = nullptr)const override;


				/// <summary>
				/// Starts asynchronous simulation
				/// </summary>
				/// <param name="deltaTime"> amount of time to simulate </param>
				virtual void SimulateAsynch(float deltaTime) override;

				/// <summary> Waits for simulation to end and fetches all intersection events </summary>
				virtual void SynchSimulation() override;

				/// <summary> Underlying API object </summary>
				operator physx::PxScene* () const;

				/// <summary> Underlying API object </summary>
				physx::PxScene* operator->()const;

				/// <summary> Scene read lock </summary>
				class ReadLock {
				private:
					// Target scene
					physx::PxScene* m_scene;

				public:
					/// <summary>
					/// Constructor
					/// </summary>
					/// <param name="scene"> Scene to lock </param>
					inline ReadLock(const PhysicsScene* scene) : m_scene(*dynamic_cast<const PhysXScene*>(scene)) { m_scene->lockRead(); }

					/// <summary> Destructor </summary>
					inline ~ReadLock() { m_scene->unlockRead(); }
				};

				/// <summary> Scene read lock </summary>
				class WriteLock {
				private:
					// Target scene
					physx::PxScene* m_scene;

				public:
					/// <summary>
					/// Constructor
					/// </summary>
					/// <param name="scene"> Scene to lock </param>
					inline WriteLock(PhysicsScene* scene) : m_scene(*dynamic_cast<PhysXScene*>(scene)) { m_scene->lockWrite(); }

					/// <summary> Destructor </summary>
					inline ~WriteLock() { m_scene->unlockWrite(); }
				};


			private:
				// Task dispatcher
				physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;

				// Underlying API object
				physx::PxScene* m_scene = nullptr;

				// Layer filter data (yep, we have an arbitrary buffer here)
				uint8_t* m_layerFilterData = nullptr;

				// True, if m_layerFilterData has been altered (cleared when SimulateAsynch() gets invoked)
				bool m_layerFilterDataDirty = true;

				// Scratch buffer
				std::vector<uint8_t> m_scratchBuffer;

				// Simulation events
				struct SimulationEventCallback : public virtual physx::PxSimulationEventCallback {
					// Simulation callbacks:
					virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
					virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override;
					virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
					virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
					virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
					virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

					// Notifies all contacts:
					void NotifyEvents();

					// Just a pair of intersecting shapes:
					struct ShapePair {
						PhysXReference<physx::PxShape> shapes[2] = { nullptr, nullptr };

						inline bool operator<(const ShapePair& info) { return shapes[0] < info.shapes[0] || (shapes[0] == info.shapes[0] && shapes[1] < info.shapes[1]); }

						struct HashEq {
							inline size_t operator()(const ShapePair& info)const {
								return MergeHashes(std::hash<const physx::PxShape*>()(info.shapes[0].operator->()), std::hash<const physx::PxShape*>()(info.shapes[1].operator->()));
							}
							inline bool operator()(const ShapePair& a, const ShapePair& b)const { return a.shapes[0] == b.shapes[0] && a.shapes[1] == b.shapes[1]; }
						};
					};

					// Information about a contact, being recorded as they come in during simulation callbacks:
					struct ContactInfo {
						PhysicsCollider::ContactType type = PhysicsCollider::ContactType::CONTACT_TYPE_COUNT;
						mutable volatile size_t firstContactPoint = 0;
						mutable volatile size_t lastContactPoint = 0;
						mutable volatile bool reverseOrder = false;
						mutable volatile uint8_t pointBuffer = 0;
					};

					// ContactInfo, paired with raw references of the shapes tied to it:
					struct ContactPairInfo {
						physx::PxShape* shapes[2] = { nullptr, nullptr };
						ContactInfo info;
					};

					// Mapping from ShapePair to ContactInfo for currently active contacts:
					typedef std::unordered_map<ShapePair, ContactInfo, ShapePair::HashEq, ShapePair::HashEq> PersistentContactMap;

					// Recorded contact state:
					std::mutex m_eventLock;
					std::vector<physx::PxContactPairPoint> m_contactPointBuffer;
					std::vector<PhysicsCollider::ContactPoint> m_contactPoints[2];
					std::vector<ContactPairInfo> m_contacts;
					std::vector<ShapePair> m_pairsToRemove;
					PersistentContactMap m_persistentContacts;
					mutable volatile uint8_t m_backBuffer = 0;

				} m_simulationEventCallback;
			};
		}
	}
}
