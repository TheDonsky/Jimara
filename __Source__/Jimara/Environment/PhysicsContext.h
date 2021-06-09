#pragma once
#include "../Physics/PhysicsInstance.h"


namespace Jimara {
	class PhysicsContext : public virtual Object {
	public:
		virtual Vector3 Gravity()const = 0;

		virtual void SetGravity(const Vector3& value) = 0;

		virtual Reference<Physics::DynamicBody> AddRigidBody(const Matrix4& transform, bool enabled = true) = 0;

		virtual Reference<Physics::StaticBody> AddStaticBody(const Matrix4& transform, bool enabled = true) = 0;

		virtual Physics::PhysicsInstance* APIInstance()const = 0;

		virtual float UpdateRate()const = 0;

		virtual void SetUpdateRate(float rate) = 0;

		virtual float ScaledDeltaTime()const = 0;

		virtual float UnscaledDeltaTime()const = 0;
	};
}
