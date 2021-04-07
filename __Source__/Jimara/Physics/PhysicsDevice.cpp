#include "PhysicsDevice.h"
#include <sstream>

namespace Jimara {
	namespace Physics {
		namespace {
			typedef Reference<PhysicsDevice>(*DeviceCreateFn)(OS::Logger*, PhysicsDevice::Backend);
		}
		
		Reference<PhysicsDevice> PhysicsDevice::Create(OS::Logger* logger, Backend backend) {
			static const DeviceCreateFn DEFAULT = [](OS::Logger* logger, Backend backend) -> Reference<PhysicsDevice> {
				std::stringstream stream;
				stream << "PhysicsDevice::Create - Unknown backend type: " << static_cast<uint32_t>(backend);
				logger->Error(stream.str());
				return nullptr;
			};
			static const DeviceCreateFn* CREATE_FUNCTIONS = [](){
				static const uint8_t CREATE_FUNCTION_COUNT = static_cast<uint8_t>(Backend::BACKEND_OPTION_COUNT);
				static DeviceCreateFn createFunctions[CREATE_FUNCTION_COUNT];
				for (uint8_t i = 0; i < CREATE_FUNCTION_COUNT; i++) createFunctions[i] = DEFAULT;
				createFunctions[static_cast<uint8_t>(Backend::NVIDIA_PHYSX)] = [](OS::Logger* logger, Backend) -> Reference<PhysicsDevice> {
					logger->Error("PhysicsDevice::Create - NVIDIA_PHYSX backend not yet implemented...");
					return nullptr;
				};
				return createFunctions;
			}();
			return ((backend < Backend::BACKEND_OPTION_COUNT) ? CREATE_FUNCTIONS[static_cast<uint8_t>(backend)] : DEFAULT)(logger, backend);
		}
	}
}
