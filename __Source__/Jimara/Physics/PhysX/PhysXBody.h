#pragma once
#include "PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// Simple wrapper on top of physx::PxRigidActor*
			/// </summary>
			class PhysXBody : public virtual PhysicsBody {
			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, this body belongs to </param>
				/// <param name="actor"> PhysX actor </param>
				/// <param name="enabled"> If true, the body will start-off enabled </param>
				PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor, bool enabled);

			public:
				/// <summary> Virtual destructor </summary>
				~PhysXBody();

				/// <summary> If true, the body is currently an active part of the scene </summary>
				virtual bool Active()const override;

				/// <summary>
				/// Activates/deactivates the body
				/// </summary>
				/// <param name="active"> If true, the body will appear in the next simulation </param>
				virtual void SetActive(bool active) override;

				/// <summary> Position and rotation of the body within the scene </summary>
				virtual Matrix4 GetPose()const override;

				/// <summary>
				/// Repositions the body
				/// </summary>
				/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				virtual void SetPose(const Matrix4& transform) override;

				/// <summary>
				/// Adds a box collider
				/// </summary>
				/// <param name="box"> Shape descriptor </param>
				/// <param name="material"> Physics material for the collider </param>
				/// <param name="listener"> Collider event listener </param>
				/// <param name="enabled"> If true, the collider will start-off enabled </param>
				/// <returns> New collider, attached to the body </returns>
				virtual Reference<PhysicsBoxCollider> AddCollider(const BoxShape& box, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) override;

				/// <summary>
				/// Adds a sphere collider
				/// </summary>
				/// <param name="sphere"> Shape descriptor </param>
				/// <param name="material"> Physics material for the collider </param>
				/// <param name="listener"> Collider event listener </param>
				/// <param name="enabled"> If true, the collider will start-off enabled </param>
				/// <returns> New collider, attached to the body </returns>
				virtual Reference<PhysicsSphereCollider> AddCollider(const SphereShape& sphere, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) override;

				/// <summary>
				/// Adds a capsule collider
				/// </summary>
				/// <param name="capsule"> Shape descriptor </param>
				/// <param name="material"> Physics material for the collider </param>
				/// <param name="listener"> Collider event listener </param>
				/// <param name="enabled"> If true, the collider will start-off enabled </param>
				/// <returns> New collider, attached to the body </returns>
				virtual Reference<PhysicsCapsuleCollider> AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) override;

				/// <summary> "Owner" scene </summary>
				PhysXScene* Scene()const;

				/// <summary> Underlying api object </summary>
				operator physx::PxRigidActor*()const;


			private:
				// "Owner" scene
				const Reference<PhysXScene> m_scene;

				// Underlying api object
				physx::PxRigidActor* m_actor = nullptr;

				// If true, the actor is currently attached to the scene
				std::atomic<bool> m_active = false;
			};
		}
	}
}
