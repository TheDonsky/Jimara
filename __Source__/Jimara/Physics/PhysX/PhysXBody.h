#pragma once
#include "PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			class PhysXBody : public virtual PhysicsBody {
			public:
				PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor);

				~PhysXBody();

				virtual bool Active()const override;

				virtual void SetActive(bool active) override;

				virtual Matrix4 GetPose()const override;

				virtual void SetPose(const Matrix4& transform) override;

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
