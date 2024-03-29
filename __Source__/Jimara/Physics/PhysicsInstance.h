#pragma once
#include "../Core/Object.h"
namespace Jimara {
	namespace Physics {
		class PhysicsInstance;
	}
}
#include "../OS/Logging/Logger.h"
#include "../Math/Math.h"
#include <thread>
#include "PhysicsScene.h"

namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Physics toolbox instance
		/// </summary>
		class JIMARA_API PhysicsInstance : public virtual Object {
		public:
			/// <summary> Available instance types, known to the engine </summary>
			enum class Backend : uint8_t {
				/// <summary> NVIDIA PhysX backend </summary>
				NVIDIA_PHYSX = 0,

				/// <summary> Not an actual backend; tells how many different backend types are available </summary>
				BACKEND_OPTION_COUNT = 1
			};

			/// <summary>
			/// Flags that can be used during scene creation
			/// </summary>
			enum class SceneCreateFlags : uint32_t {
				/// <summary> Empty flags </summary>
				NONE = 0u,

				/// <summary> 
				/// If this flag is set, scene will use preallocated scratch buffer during simulation 
				/// (May offer some speedup if applicable, but will always take more memory) 
				/// </summary>
				USE_SCRATCH_BUFFER = (1u << 0u)
			};

			/// <summary>
			/// Creates a physics toolbox instance
			/// (If underlying API supports only one instance per process, this may keep returning the same one, ignoring the logger beyond the first one)
			/// </summary>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="backend"> Physics backend </param>
			/// <returns> Physics instance </returns>
			static Reference<PhysicsInstance> Create(OS::Logger* logger, Backend backend = Backend::NVIDIA_PHYSX);

			/// <summary> Default gravity (Vector3(0.0f, -9.81f, 0.0f)) </summary>
			static Vector3 DefaultGravity();

			/// <summary>
			/// Creates a physics scene
			/// </summary>
			/// <param name="maxSimulationThreads"> Maximal number of threads the simulation can use </param>
			/// <param name="gravity"> Gravity </param>
			/// <param name="flags"> Scene Create Flags </param>
			/// <returns> New scene instance </returns>
			virtual Reference<PhysicsScene> CreateScene(
				size_t maxSimulationThreads = std::thread::hardware_concurrency(),
				const Vector3 gravity = DefaultGravity(), 
				SceneCreateFlags flags = SceneCreateFlags::USE_SCRATCH_BUFFER) = 0;

			/// <summary>
			/// Creates a physics material
			/// </summary>
			/// <param name="staticFriction"> Static friction </param>
			/// <param name="dynamicFriction"> Dynamic friction </param>
			/// <param name="bounciness"> Bounciness </param>
			/// <returns> New PhysicsMaterial instance </returns>
			virtual Reference<PhysicsMaterial> CreateMaterial(
				float staticFriction = 0.5f, float dynamicFriction = 0.5f, float bounciness = 0.5f) = 0;

			/// <summary>
			/// Creates a collision mesh for TriMesh
			/// Note: For caching to work, you should be using the CollisionMeshAsset for creation; This will always create new ones
			/// </summary>
			/// <param name="mesh"> Triangle mesh to base CollisionMesh on </param>
			/// <returns> CollisionMesh </returns>
			virtual Reference<CollisionMesh> CreateCollisionMesh(TriMesh* mesh) = 0;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			PhysicsInstance(OS::Logger* logger);

		private:
			// Logger
			const Reference<OS::Logger> m_logger;
		};


		/// <summary>
		/// Logical negation of PhysicsInstance::SceneCreateFlags
		/// </summary>
		/// <param name="f"> PhysicsInstance::SceneCreateFlags </param>
		/// <returns> ~f </returns>
		inline static PhysicsInstance::SceneCreateFlags operator~(PhysicsInstance::SceneCreateFlags f) {
			return static_cast<PhysicsInstance::SceneCreateFlags>(
				~static_cast<std::underlying_type_t<PhysicsInstance::SceneCreateFlags>>(f));
		}

		/// <summary>
		/// Logical 'or' of two PhysicsInstance::SceneCreateFlags
		/// </summary>
		/// <param name="a"> PhysicsInstance::SceneCreateFlags </param>
		/// <param name="b"> PhysicsInstance::SceneCreateFlags </param>
		/// <returns> a | b </returns>
		inline static PhysicsInstance::SceneCreateFlags operator|(PhysicsInstance::SceneCreateFlags a, PhysicsInstance::SceneCreateFlags b) {
			return static_cast<PhysicsInstance::SceneCreateFlags>(
				static_cast<std::underlying_type_t<PhysicsInstance::SceneCreateFlags>>(a) |
				static_cast<std::underlying_type_t<PhysicsInstance::SceneCreateFlags>>(b));
		}

		/// <summary>
		/// Logical 'and' of two PhysicsInstance::SceneCreateFlags
		/// </summary>
		/// <param name="a"> PhysicsInstance::SceneCreateFlags </param>
		/// <param name="b"> PhysicsInstance::SceneCreateFlags </param>
		/// <returns> a & b </returns>
		inline static PhysicsInstance::SceneCreateFlags operator&(PhysicsInstance::SceneCreateFlags a, PhysicsInstance::SceneCreateFlags b) {
			return static_cast<PhysicsInstance::SceneCreateFlags>(
				static_cast<std::underlying_type_t<PhysicsInstance::SceneCreateFlags>>(a) &
				static_cast<std::underlying_type_t<PhysicsInstance::SceneCreateFlags>>(b));
		}
	}
}
