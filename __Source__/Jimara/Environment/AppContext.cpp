#include "AppContext.h"


namespace Jimara {
	AppContext::AppContext(Graphics::GraphicsDevice* device)
		: m_device(device) {}

	OS::Logger* AppContext::Log()const { return m_device->Log(); }

	Graphics::GraphicsDevice* AppContext::GraphicsDevice()const { return m_device; }
}
