#pragma once
#include "../Physics/PhysicsInstance.h"


namespace Jimara {
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

		virtual Event<>& OnPostPhysicsSynch() = 0;

		virtual Physics::PhysicsInstance* APIInstance()const = 0;

		virtual float UpdateRate()const = 0;

		virtual void SetUpdateRate(float rate) = 0;

		virtual float ScaledDeltaTime()const = 0;

		virtual float UnscaledDeltaTime()const = 0;
	};
}
