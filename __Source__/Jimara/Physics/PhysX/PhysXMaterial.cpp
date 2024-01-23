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
				struct CachedMaterialAsset 
					: public virtual Asset::Of<PhysicsMaterial>
					, public virtual ObjectCache<Reference<Object>>::StoredObject {
					const Reference<PhysXInstance> m_instance;
					inline CachedMaterialAsset(PhysXInstance* instance) : Asset(GUID::Generate()), m_instance(instance) {}
					inline virtual ~CachedMaterialAsset() {}
					inline virtual Reference<PhysicsMaterial> LoadItem() final override { return m_instance->CreateMaterial(0.5f, 0.5f, 0.5f); }
				};
#pragma warning(default: 4250)

				class MaterialCache : public virtual ObjectCache<Reference<Object>> {
				public:
					static Reference<PhysXMaterial> GetFor(PhysXInstance* instance) {
						static MaterialCache cache;
						Reference<CachedMaterialAsset> asset = cache.GetCachedOrCreate(instance, [&]()->Reference<CachedMaterialAsset> {
							return Object::Instantiate<CachedMaterialAsset>(instance);
							});
						return asset->Load();
					}
				};

				static_assert((uint32_t)PhysicsMaterial::CombineMode::AVERAGE == (uint32_t)physx::PxCombineMode::eAVERAGE);
				static_assert((uint32_t)PhysicsMaterial::CombineMode::MIN == (uint32_t)physx::PxCombineMode::eMIN);
				static_assert((uint32_t)PhysicsMaterial::CombineMode::MULTIPLY == (uint32_t)physx::PxCombineMode::eMULTIPLY);
				static_assert((uint32_t)PhysicsMaterial::CombineMode::MAX == (uint32_t)physx::PxCombineMode::eMAX);
				static const constexpr PhysicsMaterial::CombineMode PHYSICS_MATERIAL_MAX_COMBINE_MODE =
					static_cast<PhysicsMaterial::CombineMode>(static_cast<std::underlying_type_t<PhysicsMaterial::CombineMode>>(PhysicsMaterial::CombineMode::MODE_COUNT) - 1u);
			}

			Reference<PhysXMaterial> PhysXMaterial::Default(PhysXInstance* instance) { return MaterialCache::GetFor(instance); }

			float PhysXMaterial::StaticFriction()const { return m_material->getStaticFriction(); }

			void PhysXMaterial::SetStaticFriction(float friction) { m_material->setStaticFriction(friction); }

			float PhysXMaterial::DynamicFriction()const { return m_material->getDynamicFriction(); }

			void PhysXMaterial::SetDynamicFriction(float friction) { m_material->setDynamicFriction(friction); }

			PhysicsMaterial::CombineMode PhysXMaterial::FrictionCombineMode()const { 
				return static_cast<PhysicsMaterial::CombineMode>(m_material->getFrictionCombineMode()); 
			}

			void PhysXMaterial::SetFrictionCombineMode(PhysicsMaterial::CombineMode mode) { 
				m_material->setFrictionCombineMode(static_cast<physx::PxCombineMode::Enum>(Math::Min(mode, PHYSICS_MATERIAL_MAX_COMBINE_MODE)));
			}

			float PhysXMaterial::Bounciness()const { return m_material->getRestitution(); }

			void PhysXMaterial::SetBounciness(float bounciness) { m_material->setRestitution(bounciness); }

			PhysicsMaterial::CombineMode PhysXMaterial::BouncinessCombineMode()const {
				return static_cast<PhysicsMaterial::CombineMode>(m_material->getRestitutionCombineMode());
			}

			void PhysXMaterial::SetBouncinessCombineMode(PhysicsMaterial::CombineMode mode) {
				m_material->setRestitutionCombineMode(static_cast<physx::PxCombineMode::Enum>(Math::Min(mode, PHYSICS_MATERIAL_MAX_COMBINE_MODE)));
			}

			PhysXMaterial::operator physx::PxMaterial* ()const { return m_material; }

			physx::PxMaterial* PhysXMaterial::operator->()const { return m_material; }
		}
	}
}

