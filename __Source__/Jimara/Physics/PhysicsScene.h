#pragma once
#include "PhysicsCollider.h"
namespace Jimara {
	namespace Physics {
		class PhysicsScene;
		class PhysicsInstance;
	}
}
#include "PhysicsBody.h"
#include "PhysicsInstance.h"


namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Physics scene to simulate em all
		/// </summary>
		class PhysicsScene : public virtual Object {
		public:
			/// <summary> Scene gravity </summary>
			virtual Vector3 Gravity()const = 0;

			/// <summary>
			/// Sets scene gravity
			/// </summary>
			/// <param name="value"> Gravity to use for the scene </param>
			virtual void SetGravity(const Vector3& value) = 0;

			/// <summary>
			/// Tells, if two collider layers interact
			/// </summary>
			/// <param name="a"> First layer </param>
			/// <param name="b"> Second layer </param>
			/// <returns> True, if the colliders from given layers interact </returns>
			virtual bool LayersInteract(PhysicsCollider::Layer a, PhysicsCollider::Layer b)const = 0;

			/// <summary>
			/// Marks, whether or not the colliders on given layers should interact
			/// </summary>
			/// <param name="a"> First layer </param>
			/// <param name="b"> Second layer </param>
			/// <param name="enableIntaraction"> True, if the colliders from given layers should interact </param>
			virtual void FilterLayerInteraction(PhysicsCollider::Layer a, PhysicsCollider::Layer b, bool enableIntaraction) = 0;

			/// <summary>
			/// Creates a dynimic body (ei rigidbody)
			/// </summary>
			/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
			/// <param name="enabled"> If true, the body we create will start off as enabled </param>
			/// <returns> New dynamic body </returns>
			virtual Reference<DynamicBody> AddRigidBody(const Matrix4& pose, bool enabled = true) = 0;

			/// <summary>
			/// Creates a static body (ei regular immobile obstacle)
			/// </summary>
			/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
			/// <param name="enabled"> If true, the body we create will start off as enabled </param>
			/// <returns> New static body </returns>
			virtual Reference<StaticBody> AddStaticBody(const Matrix4& pose, bool enabled = true) = 0;

			/// <summary>
			/// Starts asynchronous simulation
			/// </summary>
			/// <param name="deltaTime"> amount of time to simulate </param>
			virtual void SimulateAsynch(float deltaTime) = 0;

			/// <summary> Waits for simulation to end and fetches all intersection events </summary>
			virtual void SynchSimulation() = 0;

			/// <summary> "Owner" PhysicsInstance </summary>
			inline PhysicsInstance* APIInstance()const { return m_instance; }


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="apiInstance"> "Owner" PhysicsInstance </param>
			PhysicsScene(PhysicsInstance* apiInstance) : m_instance(apiInstance) {}

		private:
			// "Owner" PhysicsInstance
			const Reference<PhysicsInstance> m_instance;
		};
	}
}
