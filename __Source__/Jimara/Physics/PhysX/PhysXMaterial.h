#pragma once
#include "PhysXInstance.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			class PhysXMaterial : public virtual PhysicsMaterial {
			public:
				PhysXMaterial(PhysXInstance* instance, float staticFriction, float dynamicFriction, float bounciness);

				static Reference<PhysXMaterial> Default(PhysXInstance* instance);

				virtual float StaticFriction()const override;

				virtual void SetStaticFriction(float friction) override;

				virtual float DynamicFriction()const override;

				virtual void SetDynamicFriction(float friction) override;

				virtual float Bounciness()const override;

				virtual void SetBounciness(float bounciness) override;

				operator physx::PxMaterial* ()const;

				physx::PxMaterial* operator->()const;

			private:
				const Reference<PhysXInstance> m_instance;
				const PhysXReference<physx::PxMaterial> m_material;
			};
		}
	}
}
