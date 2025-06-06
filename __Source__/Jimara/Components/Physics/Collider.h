#pragma once
#include "Rigidbody.h"


namespace Jimara {
	/// <summary>
	/// Base class for all component collider types, wrapping round Physics collider objects and making them a proper part of the scene
	/// </summary>
	class JIMARA_API Collider : public virtual Component {
	public:
		/// <summary> Constructor </summary>
		Collider();

		/// <summary> Virtual destructor </summary>
		virtual ~Collider();

		/// <summary> True, if the collider is trigger </summary>
		bool IsTrigger()const;

		/// <summary>
		/// Sets trigger flag
		/// </summary>
		/// <param name="trigger"> If true, the collider will be made a trigger </param>
		void SetTrigger(bool trigger);

		/// <summary>Layer for contact filtering </summary>
		typedef Physics::PhysicsCollider::Layer Layer;

		/// <summary> Layer for contact filtering </summary>
		Layer GetLayer()const;

		/// <summary>
		/// Sets layer for contact filtering
		/// </summary>
		/// <param name="layer"> Layer to set </param>
		void SetLayer(Layer layer);

		/// <summary>
		/// Sets layer for contact filtering
		/// </summary>
		/// <typeparam name="LayerEnum"> Some enumeration, giving actual names to the layers </typeparam>
		/// <param name="layer"> Layer to set </param>
		template<typename LayerEnum>
		inline void SetLayer(const LayerEnum& layer) { SetLayer(static_cast<Layer>(layer)); }

		/// <summary>
		/// If true, the GetPhysicsCollider will be considered 'static' and it's transformation will not be synchronized On a per-frame basis.
		/// Can be used with Colliders attached to dynamic rigidbodies as well, as long as their pose inside the rigidbody stays constant.
		/// </summary>
		bool IsStatic()const;
		
		/// <summary>
		/// Marks the collider static
		/// </summary>
		/// <param name="isStatic"> If true, the collider will assume the pose in or stays constant and saves some CPU cycles doing that </param>
		void MarkStatic(bool isStatic);

		/// <summary> Collider contact event type </summary>
		using ContactType = Physics::PhysicsCollider::ContactType;

		/// <summary> Collider contact event touch point description </summary>
		using ContactPoint = Physics::PhysicsCollider::ContactPoint;

		/// <summary>
		/// Collision information
		/// </summary>
		class ContactInfo {
		private:
			// Self
			const Reference<Collider> m_collider;

			// Other
			const Reference<Collider> m_otherCollider;

			// Type
			const ContactType m_contactType;

			// Points
			const ContactPoint* const m_contactPoints;

			// Num points
			const size_t m_contactPointCount;

			// No copying allowed
			inline ContactInfo(const ContactInfo&) = delete;
			inline ContactInfo& operator=(const ContactInfo&) = delete;

		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="self"> Collider, that reported the event </param>
			/// <param name="other"> Other collider, involved in the event </param>
			/// <param name="type"> Collidion event type </param>
			/// <param name="points"> Touch points (not copied, so make sure they don't go out of scope while the info exists if you decide to create a custom instance) </param>
			/// <param name="numPoints"> Number of touch points </param>
			inline ContactInfo(Collider* self, Collider* other, ContactType type, const ContactPoint* const points, size_t numPoints)
				: m_collider(self), m_otherCollider(other), m_contactType(type), m_contactPoints(points), m_contactPointCount(numPoints) {}

			/// <summary> Collider, that reported the even </summary>
			inline Collider* ReportingCollider()const { return m_collider; }

			/// <summary> Other collider, involved in the event </summary>
			inline Collider* OtherCollider()const { return m_otherCollider; }

			/// <summary> Reason, the event was invoked </summary>
			inline ContactType EventType()const { return m_contactType; }

			/// <summary> Number of touch points </summary>
			inline size_t TouchPointCount()const { return m_contactPointCount; }

			/// <summary>
			/// Touch point by index
			/// </summary>
			/// <param name="index"> Touch point index (anything outside [0; TouchPointCount) is unsafe) </param>
			/// <returns> Index'th touch point </returns>
			inline ContactPoint TouchPoint(size_t index)const { return m_contactPoints[index]; }
		};

		/// <summary> Invoked, whenever some other collider interacts with this one </summary>
		Event<const ContactInfo&>& OnContact();

		/// <summary>
		/// "Extracts" the owner component collider if applicable
		/// </summary>
		/// <param name="collider"> Physics toolbox collider </param>
		/// <returns> Component collider </returns>
		static Collider* GetOwner(Physics::PhysicsCollider* collider);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;


	protected:
		/// <summary> Invoked by the scene on the first frame this component gets instantiated </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, whenever the component parent chain gets dirty </summary>
		virtual void OnParentChainDirty()override;

		/// <summary> Invoked, whenever the component gets destroyed </summary>
		virtual void OnComponentDestroyed()override;

		/// <summary> 
		/// Derived classes can use this method to notify the collider, that the underlying Physics::PhysicsCollider is no longer valid and 
		/// should be refreshed using GetPhysicsCollider() before it gets the chance to ruin the simulation
		/// </summary>
		void ColliderDirty();

		/// <summary>
		/// Derived classes should use this method to create and alter the underlying Physics::PhysicsCollider objects to their liking.
		/// Note: Keeping the reference to the toolbox colliders created within this method is not adviced, unless you know exactly how stuff works.
		/// </summary>
		/// <param name="old"> Collider from the previous GetPhysicsCollider() call or nullptr if it did not exist/got invalidated </param>
		/// <param name="body"> Physics body, the collider should be tied to </param>
		/// <param name="scale"> Collider scale, based on transform </param>
		/// <param name="listener"> Listener to use with this collider (always the same, so no need to check for the one tied to 'old') </param>
		/// <returns> Physics::PhysicsCollider to be used by the collider </returns>
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) = 0;

	private:
		// Main listener, associated with this collider
		const Reference<Physics::PhysicsCollider::EventListener> m_listener;

		// Attached rigidbody
		Reference<Rigidbody> m_rigidbody;

		// Body, the underlying collider is tied to
		Reference<Physics::PhysicsBody> m_body;

		// Underlying collider
		Reference<Physics::PhysicsCollider> m_collider;

		// Last pose of the collider
		Matrix4 m_lastPose = Math::Identity();

		// Last scale
		Vector3 m_lastScale = Vector3(1.0f);

		// If true, the underlying collider will be made a trigger
		std::atomic<bool> m_isTrigger = false;

		// Filtering layer
		std::atomic<Layer> m_layer = 0;

		// Static flag
		std::atomic_bool m_isStatic = false;

		// If true, GetPhysicsCollider will have to be called before the next physics synch point
		std::atomic_bool m_dirty = true;

		// Invoked, when the collider gets involved in a contact
		EventInstance<const ContactInfo&> m_onContact;

		// Updates collider state
		void SynchPhysicsCollider();

		// Notifies listeners about the contact
		void NotifyContact(const ContactInfo& info);

		// Some private stuff is here...
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Collider>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
	}


	/// <summary>
	/// Collider, that uses a single physics material
	/// </summary>
	class JIMARA_API SingleMaterialCollider : public virtual Collider {
	public:
		/// <summary> Physics material, used by the collider (nullptr means the default material) </summary>
		virtual Physics::PhysicsMaterial* Material()const = 0;

		/// <summary>
		/// Updates physics material used by the collider (nullptr will result in some default material)
		/// </summary>
		/// <param name="material"> Material to use </param>
		virtual void SetMaterial(Physics::PhysicsMaterial* material) = 0;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<SingleMaterialCollider>(const Callback<TypeId>& report) { report(TypeId::Of<Collider>()); }
}
