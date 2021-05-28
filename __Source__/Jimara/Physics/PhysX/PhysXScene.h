#pragma once
#include "PhysXInstance.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			class PhysXScene : public virtual PhysicsScene {
			public:
				PhysXScene(PhysXInstance* instance, size_t maxSimulationThreads, const Vector3 gravity);

				virtual ~PhysXScene();

				virtual Vector3 Gravity()const override;

				virtual void SetGravity(const Vector3& value) override;

				virtual Reference<RigidBody> AddRigidBody() override;

				virtual Reference<StaticBody> AddStaticBody() override;

				operator physx::PxScene* () const;

				physx::PxScene* operator->()const;
				

			private:
				physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
				physx::PxScene* m_scene = nullptr;
			};
		}
	}
}
