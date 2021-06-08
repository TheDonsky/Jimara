#pragma once
#include "PhysXInstance.h"


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
				PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXScene();

				/// <summary> Scene gravity </summary>
				virtual Vector3 Gravity()const override;

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
				

			private:
				// Task dispatcher
				physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;

				// Underlying API object
				physx::PxScene* m_scene = nullptr;

				// Simulation events
				struct SimulationEventCallback : public virtual physx::PxSimulationEventCallback {
					virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
					virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override;
					virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
					virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
					virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
					virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;
				} m_simulationEventCallback;
			};
		}
	}
}
