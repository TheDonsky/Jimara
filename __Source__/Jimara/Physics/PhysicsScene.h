#pragma once
#include "../Core/Function.h"
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
		/// Result of a raycast query
		/// </summary>
		struct RaycastHit {
			/// <summary> Collider, that got hit </summary>
			Reference<PhysicsCollider> collider = nullptr;

			/// <summary> Hit point </summary>
			Vector3 point = Vector3(0.0f, 0.0f, 0.0f);

			/// <summary> Collider's normal at the hit point </summary>
			Vector3 normal = Vector3(0.0f, 0.0f, 0.0f);
		};

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

			/// <summary> Tells, how to filter scene queries </summary>
			enum class QueryFilterFlag : uint8_t {
				/// <summary> Ignore collider hit </summary>
				DISCARD = 0,

				/// <summary> Report collider hit </summary>
				REPORT = 1,

				/// <summary> Report collider hit and prevent any intersections further than this from being reported </summary>
				REPORT_BLOCK = 2
			};

			/// <summary>
			/// Casts a ray into the scene and reports what it manages to hit
			/// </summary>
			/// <param name="origin"> Ray origin </param>
			/// <param name="direction"> Ray direction </param>
			/// <param name="maxDistance"> Max distance, the ray is allowed to travel </param>
			/// <param name="onHitFound"> If the ray hits something, this callback will be invoked with the hit info </param>
			/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
			/// <param name="reportAll"> If true, the query will report all hits without blocking, otherwise just the closest one </param>
			/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
			/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
			/// <returns> Number of reported RaycastHit-s </returns>
			virtual size_t Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
				, const Callback<const RaycastHit&>& onHitFound
				, const PhysicsCollider::LayerMask& layerMask = PhysicsCollider::LayerMask::All(), bool reportAll = false
				, const Function<QueryFilterFlag, PhysicsCollider*>* preFilter = nullptr, const Function<QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const = 0;

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
