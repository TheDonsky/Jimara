#pragma once
#include "PhysXInstance.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// A simple wrapper on top of physx::PxMaterial
			/// </summary>
			class PhysXMaterial : public virtual PhysicsMaterial {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="instance"> "Owner" physics instance </param>
				/// <param name="staticFriction"> Static friction ceofficient </param>
				/// <param name="dynamicFriction"> Dynamic friction ceofficient </param>
				/// <param name="bounciness"> Material bounciness </param>
				PhysXMaterial(PhysXInstance* instance, float staticFriction, float dynamicFriction, float bounciness);

				/// <summary>
				/// Default material instance
				/// </summary>
				/// <param name="instance"> "Owner" physics instance </param>
				/// <returns> Singleton instance of the default material, associated with the physics instance </returns>
				static Reference<PhysXMaterial> Default(PhysXInstance* instance);

				/// <summary> Static friction ceofficient </summary>
				virtual float StaticFriction()const override;

				/// <summary>
				/// Sets static friction ceofficient
				/// </summary>
				/// <param name="friction"> New value </param>
				virtual void SetStaticFriction(float friction) override;

				/// <summary> Dynamic friction ceofficient </summary>
				virtual float DynamicFriction()const override;

				/// <summary>
				/// Sets dynamic friction ceofficient
				/// </summary>
				/// <param name="friction"> New value </param>
				virtual void SetDynamicFriction(float friction) override;

				/// <summary> Material bounciness </summary>
				virtual float Bounciness()const override;

				/// <summary>
				/// Alters material bounciness
				/// </summary>
				/// <param name="bounciness"> New value (anything beyond 1.0f is 'non-physical') </param>
				virtual void SetBounciness(float bounciness) override;

				/// <summary> Underlying API object </summary>
				operator physx::PxMaterial* ()const;

				/// <summary> Underlying API object </summary>
				physx::PxMaterial* operator->()const;


			private:
				// "Owner" physics instance
				const Reference<PhysXInstance> m_instance;
				
				// Underlying API object
				const PhysXReference<physx::PxMaterial> m_material;
			};
		}
	}
}
