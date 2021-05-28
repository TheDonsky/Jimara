#include "PhysicsInstance.h"
#include "PhysX/PhysXInstance.h"

namespace Jimara {
	namespace Physics {
		Reference<PhysicsInstance> PhysicsInstance::Create(OS::Logger* logger, Backend backend) {
			typedef Reference<PhysicsInstance>(*InstanceCreateFn)(OS::Logger*, PhysicsInstance::Backend);
			
			static const InstanceCreateFn DEFAULT = [](OS::Logger* logger, Backend backend) -> Reference<PhysicsInstance> {
				logger->Error("PhysicsDevice::Create - Unknown backend type: ", static_cast<uint32_t>(backend));
				return nullptr;
			};
			
			static const InstanceCreateFn* CREATE_FUNCTIONS = [](){
				static const uint8_t CREATE_FUNCTION_COUNT = static_cast<uint8_t>(Backend::BACKEND_OPTION_COUNT);
				static InstanceCreateFn createFunctions[CREATE_FUNCTION_COUNT];
				for (uint8_t i = 0; i < CREATE_FUNCTION_COUNT; i++) createFunctions[i] = DEFAULT;
				
				createFunctions[static_cast<uint8_t>(Backend::NVIDIA_PHYSX)] = [](OS::Logger* logger, Backend) -> Reference<PhysicsInstance> {
					return Object::Instantiate<PhysX::PhysXInstance>(logger);
				};
				
				return createFunctions;
			}();
			
			return ((backend < Backend::BACKEND_OPTION_COUNT) ? CREATE_FUNCTIONS[static_cast<uint8_t>(backend)] : DEFAULT)(logger, backend);
		}

		Vector3 PhysicsInstance::DefaultGravity() { return Vector3(0.0f, -9.81f, 0.0f); }

		OS::Logger* PhysicsInstance::Log()const { return m_logger; }

		PhysicsInstance::PhysicsInstance(OS::Logger* logger) : m_logger(logger) { }
	}
}
