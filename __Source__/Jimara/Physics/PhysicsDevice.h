#pragma once
#include "../Core/Object.h"
namespace Jimara {
	namespace Physics {
		class PhysicsDevice;
	}
}
#include "../OS/Logging/Logger.h"
#include "PhysicsScene.h"

namespace Jimara {
	namespace Physics {
		class PhysicsDevice : public virtual Object {
		public:
			enum class Backend : uint8_t {
				/// <summary> PhysX backend </summary>
				NVIDIA_PHYSX = 0,

				/// <summary> Not an actual backend; tells how many different backend types are available </summary>
				BACKEND_OPTION_COUNT = 1
			};

			static Reference<PhysicsDevice> Create(OS::Logger* logger, Backend backend = Backend::NVIDIA_PHYSX);

			virtual Reference<PhysicsScene> CreateScene() = 0;
		};
	}
}
