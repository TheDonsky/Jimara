#pragma once
#include "PhysicsMaterial.h"

namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Box collider shape descriptor
		/// </summary>
		struct BoxShape {
			/// <summary> Box size </summary>
			Vector3 size;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="extents"> Box size </param>
			inline BoxShape(const Vector3& extents = Vector3(0.0f)) : size(extents) {}
		};

		/// <summary>
		/// Sphere collider shape descriptor
		/// </summary>
		struct SphereShape {
			/// <summary> Sphere radius </summary>
			float radius;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="r"> Sphere radius </param>
			inline SphereShape(float r = 0.0f) : radius(r) {}
		};

		/// <summary>
		/// Capsule collider shape descriptor
		/// </summary>
		struct CapsuleShape {
			/// <summary> Capsule end radius </summary>
			float radius;

			/// <summary> Capsule mid-section cyllinder height (not counting the end half-spheres) </summary>
			float height;

			/// <summary> Capsule alignment axis </summary>
			enum class Alignment : uint8_t {
				/// <summary> Mid section 'grows' on X axis </summary>
				X = 0,

				/// <summary> Mid section 'grows' on Y axis </summary>
				Y = 1,

				/// <summary> Mid section 'grows' on Z axis </summary>
				Z = 2
			} alignment;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="r"> Capsule end radius </param>
			/// <param name="h"> Capsule mid-section cyllinder height (not counting the end half-spheres) </param>
			/// <param name="a"> Capsule alignment </param>
			inline CapsuleShape(float r = 0.0f, float h = 0.0f, Alignment a = Alignment::Y) : radius(r), height(h), alignment(a) {}
		};

		/// <summary>
		/// Collider/Trigger
		/// </summary>
		class PhysicsCollider : public virtual Object {
		public:
			/// <summary> If true, the collider is currently active and attached to the corresponding body </summary>
			virtual bool Active()const = 0;

			/// <summary>
			/// Enables or disables the collider
			/// </summary>
			/// <param name="active"> If true, the collider will get enabled </param>
			virtual void SetActive(bool active) = 0;

			/// <summary> Local pose of the collider within the body </summary
			virtual Matrix4 GetLocalPose()const = 0;

			/// <summary>
			/// Sets local pose of the collider within the body
			/// </summary>
			/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
			virtual void SetLocalPose(const Matrix4& transform) = 0;

			/// <summary> True, if the collider is trigger </summary>
			virtual bool IsTrigger()const = 0;

			/// <summary>
			/// Sets trigger flag
			/// </summary>
			/// <param name="trigger"> If true, the collider will be made a trigger </param>
			virtual void SetTrigger(bool trigger) = 0;
		};

		/// <summary>
		/// Box collider/trigger
		/// </summary>
		class PhysicsBoxCollider : public virtual PhysicsCollider {
		public:
			/// <summary>
			/// Alters collider shape
			/// </summary>
			/// <param name="newShape"> Updated shape </param>
			virtual void Update(const BoxShape& newShape) = 0;
		};

		/// <summary>
		/// Sphere collider/trigger
		/// </summary>
		class PhysicsSphereCollider : public virtual PhysicsCollider {
		public:
			/// <summary>
			/// Alters collider shape
			/// </summary>
			/// <param name="newShape"> Updated shape </param>
			virtual void Update(const SphereShape& newShape) = 0;
		};

		/// <summary>
		/// Capsule collider/trigger
		/// </summary>
		class PhysicsCapsuleCollider : public virtual PhysicsCollider {
		public:
			/// <summary>
			/// Alters collider shape
			/// </summary>
			/// <param name="newShape"> Updated shape </param>
			virtual void Update(const CapsuleShape& newShape) = 0;
		};
	}
}
