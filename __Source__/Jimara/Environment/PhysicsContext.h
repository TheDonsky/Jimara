#pragma once
#include "../Physics/PhysicsInstance.h"


namespace Jimara {
	class Collider;

	/// <summary>
	/// Result of a raycast query
	/// </summary>
	struct RaycastHit {
		/// <summary> Collider, that got hit </summary>
		Reference<Collider> collider = nullptr;

		/// <summary> Hit point </summary>
		Vector3 point = Vector3(0.0f, 0.0f, 0.0f);

		/// <summary> Collider's normal at the hit point </summary>
		Vector3 normal = Vector3(0.0f, 0.0f, 0.0f);

		/// <summary> Distance, the query traveled for </summary>
		float distance = 0;
	};

	class PhysicsContext : public virtual Object {
	public:
		virtual Vector3 Gravity()const = 0;

		virtual void SetGravity(const Vector3& value) = 0;

		/// <summary>
		/// Tells, if two collider layers interact
		/// </summary>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <returns> True, if the colliders from given layers interact </returns>
		virtual bool LayersInteract(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b)const = 0;

		template<typename LayerType>
		inline bool LayersInteract(const LayerType& a, const LayerType& b) {
			return LayersInteract(static_cast<Physics::PhysicsCollider::Layer>(a), static_cast<Physics::PhysicsCollider::Layer>(b));
		}

		/// <summary>
		/// Marks, whether or not the colliders on given layers should interact
		/// </summary>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <param name="enableIntaraction"> True, if the colliders from given layers should interact </param>
		virtual void FilterLayerInteraction(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b, bool enableIntaraction) = 0;

		template<typename LayerType>
		inline void FilterLayerInteraction(const LayerType& a, const LayerType& b, bool enableIntaraction) {
			FilterLayerInteraction(static_cast<Physics::PhysicsCollider::Layer>(a), static_cast<Physics::PhysicsCollider::Layer>(b), enableIntaraction);
		}

		virtual Reference<Physics::DynamicBody> AddRigidBody(const Matrix4& transform, bool enabled = true) = 0;

		virtual Reference<Physics::StaticBody> AddStaticBody(const Matrix4& transform, bool enabled = true) = 0;

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
			, const Physics::PhysicsCollider::LayerMask& layerMask = Physics::PhysicsCollider::LayerMask::All(), bool reportAll = false
			, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter = nullptr
			, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const = 0;

		virtual Event<>& OnPostPhysicsSynch() = 0;

		virtual Physics::PhysicsInstance* APIInstance()const = 0;

		virtual float UpdateRate()const = 0;

		virtual void SetUpdateRate(float rate) = 0;

		virtual float ScaledDeltaTime()const = 0;

		virtual float UnscaledDeltaTime()const = 0;
	};
}
