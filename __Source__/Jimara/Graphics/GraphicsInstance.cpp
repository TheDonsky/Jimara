#include "GraphicsInstance.h"
#include "Vulkan/VulkanInstance.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			typedef Reference<GraphicsInstance>(*InstanceCreateFn)(OS::Logger* logger);

			template<typename InstanceType>
			Reference<GraphicsInstance> CreateInstance(OS::Logger* logger) { return Object::Instantiate<InstanceType>(logger); }

			static const InstanceCreateFn InstanceCreateFunction(GraphicsInstance::Backend backend) {
				static const InstanceCreateFn DEFAULT = [](OS::Logger*) -> Reference<GraphicsInstance> { return Reference<GraphicsInstance>(nullptr); };
				static const InstanceCreateFn* CREATE_FUNCTIONS = []() {
					static const uint8_t BACKEND_OPTION_COUNT = static_cast<uint8_t>(GraphicsInstance::Backend::BACKEND_OPTION_COUNT);
					static InstanceCreateFn functions[BACKEND_OPTION_COUNT];
					for (int i = 0; i < BACKEND_OPTION_COUNT; i++) functions[i] = DEFAULT;
					functions[static_cast<uint8_t>(GraphicsInstance::Backend::VULKAN)] = CreateInstance<Vulkan::VulkanInstance>;
					return functions;
				}();
				return backend < GraphicsInstance::Backend::BACKEND_OPTION_COUNT ? CREATE_FUNCTIONS[static_cast<uint8_t>(backend)] : DEFAULT;
			}
		}

		Reference<GraphicsInstance> GraphicsInstance::Create(OS::Logger* logger, Backend backend) {
			return InstanceCreateFunction(backend)(logger);
		}

		GraphicsInstance::~GraphicsInstance() {}

		OS::Logger* GraphicsInstance::GraphicsInstance::Log()const { return m_logger; }

		GraphicsInstance::GraphicsInstance(OS::Logger* logger) : m_logger(logger) { }
	}
}
