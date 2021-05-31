#include "PhysXMaterial.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXMaterial::PhysXMaterial(PhysXInstance* instance, float staticFriction, float dynamicFriction, float bounciness) 
				: m_instance(instance), m_material((*instance)->createMaterial(staticFriction, dynamicFriction, bounciness)) {
				if (m_material == nullptr) m_instance->Log()->Fatal("PhysXMaterial::PhysXMaterial - Failed to create material!");
				else m_material->release();
			}

			float PhysXMaterial::StaticFriction()const { return m_material->getStaticFriction(); }

			void PhysXMaterial::SetStaticFriction(float friction) { m_material->setStaticFriction(friction); }

			float PhysXMaterial::DynamicFriction()const { return m_material->getDynamicFriction(); }

			void PhysXMaterial::SetDynamicFriction(float friction) { m_material->setDynamicFriction(friction); }

			float PhysXMaterial::Bounciness()const { return m_material->getRestitution(); }

			void PhysXMaterial::SetBounciness(float bounciness) { m_material->setRestitution(bounciness); }

			PhysXMaterial::operator physx::PxMaterial* ()const { return m_material; }

			physx::PxMaterial* PhysXMaterial::operator->()const { return m_material; }
		}
	}
}

