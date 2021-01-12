#include "GraphicsInstance.h"


namespace Jimara {
	namespace Graphics {
		Reference<GraphicsInstance> GraphicsInstance::Create(OS::Logger* logger, Backend backend) {
			return nullptr;
		}

		GraphicsInstance::~GraphicsInstance() {}

		OS::Logger* GraphicsInstance::GraphicsInstance::Log()const { return m_logger; }

		GraphicsInstance::GraphicsInstance(OS::Logger* logger) : m_logger(logger) { }
	}
}
