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
		class PhysicsInstance : public virtual Object {
		public:
			enum class Backend : uint8_t {
				/// <summary> PhysX backend </summary>
				NVIDIA_PHYSX = 0,

				/// <summary> Not an actual backend; tells how many different backend types are available </summary>
				BACKEND_OPTION_COUNT = 1
			};

			static Reference<PhysicsInstance> Create(OS::Logger* logger, Backend backend = Backend::NVIDIA_PHYSX);

			static Vector3 DefaultGravity();

			virtual Reference<PhysicsScene> CreateScene(
				size_t maxSimulationThreads = std::thread::hardware_concurrency(),
				const Vector3 gravity = DefaultGravity()) = 0;

			virtual Reference<PhysicsMaterial> CreateMaterial(
				float staticFriction = 0.5f, float dynamicFriction = 0.5f, float bounciness = 0.5f) = 0;

			OS::Logger* Log()const;

		protected:
			PhysicsInstance(OS::Logger* logger);

		private:
			const Reference<OS::Logger> m_logger;
		};
	}
}
