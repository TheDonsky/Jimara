#pragma once
#include "PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			class PhysXBody : public virtual PhysicsBody {
			public:
				PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor, bool enabled);

				~PhysXBody();

				virtual bool Active()const override;

				virtual void SetActive(bool active) override;

				virtual Matrix4 GetPose()const override;

				virtual void SetPose(const Matrix4& transform) override;

				virtual Reference<PhysicsCollider> AddCollider(const BoxShape& box, PhysicsMaterial* material, bool enabled) override;

				virtual Reference<PhysicsCollider> AddCollider(const SphereShape& sphere, PhysicsMaterial* material, bool enabled) override;

				virtual Reference<PhysicsCollider> AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, bool enabled) override;

				PhysXScene* Scene()const;

				operator physx::PxRigidActor*()const;


			private:
				const Reference<PhysXScene> m_scene;
				physx::PxRigidActor* m_actor = nullptr;
				std::atomic<bool> m_active = false;
			};
		}
	}
}
