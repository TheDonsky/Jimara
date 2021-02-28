#include "AppContext.h"


namespace Jimara {
	AppContext::AppContext(Graphics::GraphicsDevice* device)
		: m_device(device), m_graphicsMeshCache(Object::Instantiate<Cache::GraphicsMeshCache>(device)) {}

	OS::Logger* AppContext::Log()const { return m_device->Log(); }

	Graphics::GraphicsDevice* AppContext::GraphicsDevice()const { return m_device; }

	Cache::GraphicsMeshCache* AppContext::GraphicsMeshCache()const { return m_graphicsMeshCache; }
}
