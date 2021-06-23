#include "AudioInstance.h"
#include "OpenAL/OpenALInstance.h"

namespace Jimara {
	namespace Audio {
		Reference<AudioInstance> AudioInstance::Create(OS::Logger* logger, Backend backend) {
			typedef Reference<AudioInstance>(*InstanceCreateFn)(OS::Logger*, Backend);
			static const InstanceCreateFn UNKNOWN = [](OS::Logger* logger, Backend backend) -> Reference<AudioInstance> {
				logger->Error("AudioInstance::Create - Unknown Backend type: ", static_cast<size_t>(backend));
				return nullptr;
			};
			static const InstanceCreateFn* const CREATE_FN = []() -> InstanceCreateFn* {
				static const size_t BACKEND_COUNT = static_cast<size_t>(Backend::BACKEND_COUNT);
				static InstanceCreateFn createFn[BACKEND_COUNT];
				for (size_t i = 0; i < BACKEND_COUNT; i++) createFn[i] = UNKNOWN;
				createFn[static_cast<size_t>(Backend::OPEN_AL)] = [](OS::Logger* logger, Backend) -> Reference<AudioInstance> {
					return Object::Instantiate<OpenAL::OpenALInstance>(logger);
				};
				return createFn;
			}();
			const InstanceCreateFn& fn = (backend < Backend::BACKEND_COUNT) ? CREATE_FN[static_cast<size_t>(backend)] : UNKNOWN;
			return fn(logger, backend);
		}
	}
}
