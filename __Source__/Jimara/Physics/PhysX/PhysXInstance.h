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
				/// <param name="flags"> Scene Create Flags </param>
				/// <returns> New scene instance </returns>
				virtual Reference<PhysicsScene> CreateScene(size_t maxSimulationThreads, const Vector3 gravity, SceneCreateFlags flags) override;

				/// <summary>
				/// Creates a physics material
				/// </summary>
				/// <param name="staticFriction"> Static friction </param>
				/// <param name="dynamicFriction"> Dynamic friction </param>
				/// <param name="bounciness"> Bounciness </param>
				/// <returns> New PhysicsMaterial instance </returns>
				virtual Reference<PhysicsMaterial> CreateMaterial(float staticFriction, float dynamicFriction, float bounciness) override;

				/// <summary>
				/// Creates a collision mesh for TriMesh
				/// Note: For caching to work, you should be using the CollisionMeshAsset for creation; This will always create new ones
				/// </summary>
				/// <param name="mesh"> Triangle mesh to base CollisionMesh on </param>
				/// <returns> CollisionMesh </returns>
				virtual Reference<CollisionMesh> CreateCollisionMesh(TriMesh* mesh) override;

				/// <summary> Underlying API object </summary>
				operator physx::PxPhysics* () const;

				/// <summary> Underlying API object </summary>
				physx::PxPhysics* operator->()const;

				/// <summary> Main Cooking instance </summary>
				physx::PxCooking* Cooking()const;


			private:
				// Wrapper on the actual singleton instance
				const Reference<Object> m_instance;
			};
		}
	}
}
