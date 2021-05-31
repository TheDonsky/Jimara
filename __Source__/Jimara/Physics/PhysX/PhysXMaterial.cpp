#include "PhysXMaterial.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXMaterial::PhysXMaterial(PhysXInstance* instance, float staticFriction, float dynamicFriction, float bounciness) 
				: m_instance(instance), m_material((*instance)->createMaterial(staticFriction, dynamicFriction, bounciness)) {
				if (m_material == nullptr) m_instance->Log()->Fatal("PhysXMaterial::PhysXMaterial - Failed to create material!");
				else m_material->release();
			}

			namespace {
#pragma warning(disable: 4250)
				class CachedMaterial : public virtual PhysXMaterial, public virtual ObjectCache<Reference<Object>>::StoredObject {
				public:
					inline CachedMaterial(PhysXInstance* instance, float staticFriction = 0.5f, float dynamicFriction = 0.5f, float bounciness = 0.5f)
						: PhysXMaterial(instance, staticFriction, dynamicFriction, bounciness) {}
				};
#pragma warning(default: 4250)

				class MaterialCache : public virtual ObjectCache<Reference<Object>> {
				public:
					static Reference<PhysXMaterial> GetFor(PhysXInstance* instance) {
						static MaterialCache cache;
						return cache.GetCachedOrCreate(instance, false, [&]()->Reference<PhysXMaterial> { return Object::Instantiate<CachedMaterial>(instance); });
					}
				};
			}

			Reference<PhysXMaterial> PhysXMaterial::Default(PhysXInstance* instance) { return MaterialCache::GetFor(instance); }

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

