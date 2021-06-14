#pragma once
#include "PhysXBody.h"
#include "PhysXMaterial.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// PhysX collider
			/// </summary>
			class PhysXCollider : public virtual PhysicsCollider {
			public:
				/// <summary> Virtual destructor </summary>
				virtual ~PhysXCollider();

				/// <summary> If true, the collider is currently active and attached to the corresponding body </summary>
				virtual bool Active()const override;

				/// <summary>
				/// Enables or disables the collider
				/// </summary>
				/// <param name="active"> If true, the collider will get enabled </param>
				virtual void SetActive(bool active) override;

				/// <summary> Local pose of the collider within the body </summary
				virtual Matrix4 GetLocalPose()const override;

				/// <summary>
				/// Sets local pose of the collider within the body
				/// </summary>
				/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				virtual void SetLocalPose(const Matrix4& transform) override;

				/// <summary> True, if the collider is trigger </summary>
				virtual bool IsTrigger()const override;

				/// <summary>
				/// Sets trigger flag
				/// </summary>
				/// <param name="trigger"> If true, the collider will be made a trigger </param>
				virtual void SetTrigger(bool trigger) override;

				/// <summary> "Owner" body </summary>
				PhysXBody* Body()const;

				/// <summary> Underlying shape </summary>
				physx::PxShape* Shape()const;

				/// <summary>
				/// UserData for PhysX collider shape objects
				/// </summary>
				class UserData {
				public:
					/// <summary>
					/// Invoked, when the scene detects a contact event during simulation
					/// </summary>
					/// <param name="shape"> Shape, the event got triggered on </param>
					/// <param name="otherShape"> Other shape involved </param>
					/// <param name="type"> Contact event type </param>
					/// <param name="points"> Contact points </param>
					/// <param name="pointCount"> Number of contact points </param>
					void OnContact(
						physx::PxShape* shape, physx::PxShape* otherShape, PhysicsCollider::ContactType type
						, const PhysicsCollider::ContactPoint* points, size_t pointCount);

					/// <summary> Owner collider </summary>
					PhysicsCollider* Collider()const;

				private:
					// Owner
					PhysXCollider* m_owner = nullptr;

					// Owner has to have unrestricted access to internals
					friend class PhysXCollider;
				};


				/// <summary> Filter flags, that may be set as the last word of physx::PxFilterData </summary>
				enum FilterFlag : uint32_t {
					/// <summary> This flag means simulated trigger </summary>
					IS_TRIGGER = (1 << 0)
				};

				/// <summary> FilterFlag bitmask </summary>
				typedef uint32_t FilterFlags;

				/// <summary> Extracts filter flags from physx::PxFilterData </summary>
				static PX_INLINE FilterFlags GetFilterFlags(const physx::PxFilterData& data) { return data.word3; }


			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="body"> "Owner" body </param>
				/// <param name="shape"> Underlying shape </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will be created 'enabled' from the get go </param>
				PhysXCollider(PhysXBody* body, physx::PxShape* shape, PhysicsCollider::EventListener* listener, bool active);

				// Notifies that the collider has been destroyed
				void Destroyed();

			private:
				// "Owner" body
				const Reference<PhysXBody> m_body;

				// Underlying shape
				const PhysXReference<physx::PxShape> m_shape;

				// Filter data (ei layers and flags)
				physx::PxFilterData m_filterData = physx::PxFilterData(0u, 0u, 0u, 0u);

				// User data
				UserData m_userData;

				// True, if currently attached to the body
				std::atomic<bool> m_active = false;
			};


#pragma warning(disable: 4250)
			/// <summary>
			/// PhysX collider, that has a single material attacked to it
			/// </summary>
			class SingleMaterialPhysXCollider : public virtual PhysXCollider, public virtual SingleMaterialCollider {
			public:
				/// <summary>
				/// Currently set material
				/// (nullptr is never returned; if material, set bu the user is nullptr, the system should pick the globally available default material)
				/// </summary>
				virtual PhysicsMaterial* Material()const override;

				/// <summary>
				/// Sets material
				/// </summary>
				/// <param name="material"> Material to set (nullptr means default material) </param>
				virtual void SetMaterial(PhysicsMaterial* material) override;


			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="body"> "Owner" body </param>
				/// <param name="shape"> Underlying shape </param>
				/// <param name="material"> Physics material to use </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will be created 'enabled' from the get go </param>
				SingleMaterialPhysXCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active);


			private:
				// Currently used material
				Reference<PhysXMaterial> m_material;
			};



			/// <summary>
			/// PhysX-backed box collider
			/// </summary>
			class PhysXBoxCollider : public virtual SingleMaterialPhysXCollider, public virtual PhysicsBoxCollider {
			public:
				/// <summary>
				/// Creates a collider
				/// </summary>
				/// <param name="body"> Body to attach shape to </param>
				/// <param name="geometry"> Geometry descriptor </param>
				/// <param name="material"> Material to use </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will start off enabled </param>
				/// <returns> New instance of a collider </returns>
				static Reference<PhysXBoxCollider> Create(PhysXBody* body, const BoxShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXBoxCollider();

				/// <summary>
				/// Alters collider shape
				/// </summary>
				/// <param name="newShape"> Updated shape </param>
				virtual void Update(const BoxShape& newShape) override;

			private:
				// Constructor
				PhysXBoxCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				// Translates BoxShape into PxBoxGeometry
				static physx::PxBoxGeometry Geometry(const BoxShape& shape);
			};



			/// <summary>
			/// PhysX-backed sphere collider
			/// </summary>
			class PhysXSphereCollider : public virtual SingleMaterialPhysXCollider, public virtual PhysicsSphereCollider {
			public:
				/// <summary>
				/// Creates a collider
				/// </summary>
				/// <param name="body"> Body to attach shape to </param>
				/// <param name="geometry"> Geometry descriptor </param>
				/// <param name="material"> Material to use </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will start off enabled </param>
				/// <returns> New instance of a collider </returns>
				static Reference<PhysXSphereCollider> Create(PhysXBody* body, const SphereShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXSphereCollider();

				/// <summary>
				/// Alters collider shape
				/// </summary>
				/// <param name="newShape"> Updated shape </param>
				virtual void Update(const SphereShape& newShape) override;

			private:
				// Constructor
				PhysXSphereCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				// Translates SphereShape into PxSphereGeometry
				static physx::PxSphereGeometry Geometry(const SphereShape& shape);
			};



			/// <summary>
			/// PhysX-backed capsule collider
			/// </summary>
			class PhysXCapusuleCollider : public virtual SingleMaterialPhysXCollider, public virtual PhysicsCapsuleCollider {
			public:
				/// <summary>
				/// Creates a collider
				/// </summary>
				/// <param name="body"> Body to attach shape to </param>
				/// <param name="geometry"> Geometry descriptor </param>
				/// <param name="material"> Material to use </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will start off enabled </param>
				/// <returns> New instance of a collider </returns>
				static Reference<PhysXCapusuleCollider> Create(PhysXBody* body, const CapsuleShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXCapusuleCollider();

				/// <summary>
				/// Alters collider shape
				/// </summary>
				/// <param name="newShape"> Updated shape </param>
				virtual void Update(const CapsuleShape& newShape) override;

				/// <summary> Local pose of the collider within the body </summary
				virtual Matrix4 GetLocalPose()const override;

				/// <summary>
				/// Sets local pose of the collider within the body
				/// </summary>
				/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				virtual void SetLocalPose(const Matrix4& transform) override;


			private:
				// Constructor
				PhysXCapusuleCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, const CapsuleShape& capsule);

				// Translates CapsuleShape into PxCapsuleGeometry
				static physx::PxCapsuleGeometry Geometry(const CapsuleShape& shape);

				// Updates alignment
				void SetAlignment(CapsuleShape::Alignment alignment);

				// Alignment pose 'wrangler' and 'dewrangler' matrix pair
				typedef std::pair<Matrix4, Matrix4> Wrangler;

				// Alignment pose 'wrangler' and 'dewrangler' matrix pair per alignment
				static Wrangler Wrangle(CapsuleShape::Alignment alignment);

				// Initial wrangler
				Wrangler m_wrangle = Wrangle(CapsuleShape::Alignment::X);
			};
#pragma warning(default: 4250)
		}
	}
}
