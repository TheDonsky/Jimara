#pragma once
#include "PhysicsMaterial.h"

namespace Jimara {
	namespace Physics {
		struct BoxShape {
			Vector3 size;

			inline BoxShape(const Vector3& extents = Vector3(0.0f)) : size(extents) {}
		};

		struct SphereShape {
			float radius;

			inline SphereShape(float r = 0.0f) : radius(r) {}
		};

		struct CapsuleShape {
			float radius;
			float height;

			inline CapsuleShape(float r = 0.0f, float h = 0.0f) : radius(r), height(h) {}
		};

		class Collider : public virtual Object {
		public:
			virtual bool Active()const = 0;

			virtual void SetActive(bool active) = 0;

			virtual Matrix4 GetLocalPose()const = 0;

			virtual void SetLocalPose(const Matrix4& transform) = 0;
		};

		class PhysicsBody : public virtual Object {
		public:
			virtual bool Active()const = 0;

			virtual void SetActive(bool active) = 0;

			virtual Matrix4 GetPose()const = 0;

			virtual void SetPose(const Matrix4& transform) = 0;

			virtual Reference<Collider> AddCollider(const BoxShape& box, PhysicsMaterial* material) = 0;

			virtual Reference<Collider> AddCollider(const SphereShape& sphere, PhysicsMaterial* material) = 0;

			virtual Reference<Collider> AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material) = 0;
		};
	}
}
