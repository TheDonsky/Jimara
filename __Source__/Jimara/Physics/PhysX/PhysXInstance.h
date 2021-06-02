#pragma once
#include "../PhysicsInstance.h"
#include "PhysXAPIIncludes.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// NVIDIA PhysX instance wrapper
			/// </summary>
			class PhysXInstance : public virtual PhysicsInstance {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="logger"> Logger </param>
				PhysXInstance(OS::Logger* logger);

				/// <summary>
				/// Creates a physics scene
				/// </summary>
				/// <param name="maxSimulationThreads"> Maximal number of threads the simulation can use </param>
				/// <param name="gravity"> Gravity </param>
				/// <returns> New scene instance </returns>
				virtual Reference<PhysicsScene> CreateScene(size_t maxSimulationThreads, const Vector3 gravity) override;

				/// <summary>
				/// Creates a physics material
				/// </summary>
				/// <param name="staticFriction"> Static friction </param>
				/// <param name="dynamicFriction"> Dynamic friction </param>
				/// <param name="bounciness"> Bounciness </param>
				/// <returns> New PhysicsMaterial instance </returns>
				virtual Reference<PhysicsMaterial> CreateMaterial(float staticFriction, float dynamicFriction, float bounciness) override;

				/// <summary> Underlying API object </summary>
				operator physx::PxPhysics* () const;

				/// <summary> Underlying API object </summary>
				physx::PxPhysics* operator->()const;


			private:
				// Wrapper on the actual singleton instance
				const Reference<Object> m_instance;
			};
		}
	}
}
