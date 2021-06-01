#include "AppContext.h"


namespace Jimara {
	AppContext::AppContext(Graphics::GraphicsDevice* device, Physics::PhysicsInstance* physics)
		: m_device(device)
		, m_shaderCache(Graphics::ShaderCache::ForDevice(device))
		, m_graphicsMeshCache(Object::Instantiate<Graphics::GraphicsMeshCache>(device))
		, m_physicsInstance(physics != nullptr ? Reference<Physics::PhysicsInstance>(physics) : Physics::PhysicsInstance::Create(device->Log())) {}

	OS::Logger* AppContext::Log()const { return m_device->Log(); }

	Graphics::GraphicsDevice* AppContext::GraphicsDevice()const { return m_device; }

	Graphics::ShaderCache* AppContext::ShaderCache()const { return m_shaderCache; }

	Graphics::GraphicsMeshCache* AppContext::GraphicsMeshCache()const { return m_graphicsMeshCache; }

	Physics::PhysicsInstance* AppContext::PhysicsInstance()const { return m_physicsInstance; }
}
