#pragma once
#include "PhysicsMaterial.h"
#include "../Math/BitMask.h"
#include "../Data/Mesh.h"

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
		/// Mesh collider shape descriptor
		/// </summary>
		struct MeshShape {
			/// <summary> Mesh, used by the collider </summary>
			Reference<const TriMesh> mesh;

			/// <summary> Mesh scale </summary>
			Vector3 scale;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="m"> Mesh </param>
			/// <param name="s"> Collider scale </param>
			inline MeshShape(const TriMesh* m = nullptr, const Vector3& s = Vector3(1.0f)) : mesh(m), scale(s) {}
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

			/// <summary> Layer for contact filtering </summary>
			typedef uint8_t Layer;

			/// <summary> Layer mask for collider layers </summary>
			typedef BitMask<Layer> LayerMask;

			/// <summary> Layer for contact filtering </summary>
			virtual Layer GetLayer()const = 0;

			/// <summary>
			/// Sets layer for contact filtering
			/// </summary>
			/// <param name="layer"> Layer to set </param>
			virtual void SetLayer(Layer layer) = 0;


			/// <summary>
			/// Type of a contact between two colliders
			/// </summary>
			enum class ContactType : uint8_t {
				/// <summary> Colliders just touched </summary>
				ON_COLLISION_BEGIN = 0,

				/// <summary> Colliders are "keeping in touch" </summary>
				ON_COLLISION_PERSISTS = 1,

				/// <summary> Collider touch lost </summary>
				ON_COLLISION_END = 2,

				/// <summary> Colliders just touched and at least one of them is a trigger </summary>
				ON_TRIGGER_BEGIN = 3,

				/// <summary> Colliders are "keeping in touch" and at least one of them is a trigger </summary>
				ON_TRIGGER_PERSISTS = 4,

				/// <summary> Collider touch lost and at least one of them is a trigger </summary>
				ON_TRIGGER_END = 5,

				/// <summary> Nuber of types withing the enumeration </summary>
				CONTACT_TYPE_COUNT
			};

			/// <summary>
			/// Collision contact point information
			/// </summary>
			struct ContactPoint {
				/// <summary> Point, the colliders "share" during the contact </summary>
				Vector3 position;

				/// <summary> Surface normal of the other collider at touch position </summary>
				Vector3 normal;
			};

			/// <summary>
			/// Interface, for providing the information about collision events
			/// </summary>
			class ContactInfo {
			public:
				/// <summary> Collider, reporting the event </summary>
				virtual PhysicsCollider* Collider()const = 0;

				/// <summary> Other collider, involved in the event </summary>
				virtual PhysicsCollider* OtherCollider()const = 0;

				/// <summary> Tells, what type of event this info describes </summary>
				virtual ContactType EventType()const = 0;

				/// <summary> Number of contact points reported (may be 0, under some circumstances) </summary>
				virtual size_t ContactPointCount()const = 0;

				/// <summary>
				/// Contact point info by index
				/// </summary>
				/// <param name="index"> Contact point index </param>
				/// <returns> Contact point information </returns>
				virtual PhysicsCollider::ContactPoint ContactPoint(size_t index)const = 0;
			};

			/// <summary>
			/// Object, that listens to collider-related events that get reported
			/// </summary>
			class EventListener : public virtual Object {
			protected:
				/// <summary>
				/// Invoked, when some other collider directly interacts with the one, holding the listener
				/// Note: Do not expect trigger events to have any contact points, while collider ones will more than likely contain at least one
				/// </summary>
				/// <param name="info"> Contact information </param>
				virtual void OnContact(const ContactInfo& info) = 0;

				/// <summary> Only colliders are permitted to invoke listener callbacks </summary>
				friend class PhysicsCollider;
			};

			/// <summary> Listener, that listens to this collider </summary>
			inline EventListener* Listener()const { return m_listener; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="listener"> Event listener </param>
			inline PhysicsCollider(EventListener* listener) : m_listener(listener) {}

			/// <summary>
			/// Notifies the listener about the contact
			/// </summary>
			/// <param name="info"> Contact information </param>
			inline void NotifyContact(const ContactInfo& info) { if (m_listener != nullptr) m_listener->OnContact(info); }

		private:
			// Listener
			const Reference<EventListener> m_listener;
		};

		/// <summary>
		/// Collider, that can have only one material on it
		/// </summary>
		class SingleMaterialCollider : public virtual PhysicsCollider {
		public:
			/// <summary>
			/// Currently set material
			/// (nullptr is never returned; if material, set bu the user is nullptr, the system should pick the globally available default material)
			/// </summary>
			virtual PhysicsMaterial* Material()const = 0;

			/// <summary>
			/// Sets material
			/// </summary>
			/// <param name="material"> Material to set (nullptr means default material) </param>
			virtual void SetMaterial(PhysicsMaterial* material) = 0;
		};

		/// <summary>
		/// Box collider/trigger
		/// </summary>
		class PhysicsBoxCollider : public virtual SingleMaterialCollider {
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
		class PhysicsSphereCollider : public virtual SingleMaterialCollider {
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
		class PhysicsCapsuleCollider : public virtual SingleMaterialCollider {
		public:
			/// <summary>
			/// Alters collider shape
			/// </summary>
			/// <param name="newShape"> Updated shape </param>
			virtual void Update(const CapsuleShape& newShape) = 0;
		};

		/// <summary>
		/// Mesh collider/trigger
		/// </summary>
		class PhysicsMeshCollider : public virtual SingleMaterialCollider {
			/// <summary>
			/// Alters collider shape
			/// </summary>
			/// <param name="newShape"> Updated shape </param>
			virtual void Update(const MeshShape& newShape) = 0;
		};
	}
}
