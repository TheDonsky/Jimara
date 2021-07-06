#include "AppContext.h"


namespace Jimara {
	AppContext::AppContext(Graphics::GraphicsDevice* device, Physics::PhysicsInstance* physics, Audio::AudioDevice* audioDevice)
		: m_device(device)
		, m_shaderCache(Graphics::ShaderCache::ForDevice(device))
		, m_graphicsMeshCache(Object::Instantiate<Graphics::GraphicsMeshCache>(device))
		, m_physicsInstance(physics != nullptr ? Reference<Physics::PhysicsInstance>(physics) : Physics::PhysicsInstance::Create(device->Log()))
		, m_audioDevice([&]() -> Reference<Audio::AudioDevice> {
		if (audioDevice != nullptr) return audioDevice;
		Reference<Audio::AudioInstance> instance = Audio::AudioInstance::Create(device->Log());
		return instance->DefaultDevice()->CreateLogicalDevice();
			}()) {}

	OS::Logger* AppContext::Log()const { return m_device->Log(); }

	Graphics::GraphicsDevice* AppContext::GraphicsDevice()const { return m_device; }

	Graphics::ShaderCache* AppContext::ShaderCache()const { return m_shaderCache; }

	Graphics::GraphicsMeshCache* AppContext::GraphicsMeshCache()const { return m_graphicsMeshCache; }

	Physics::PhysicsInstance* AppContext::PhysicsInstance()const { return m_physicsInstance; }

	Audio::AudioDevice* AppContext::AudioDevice()const { return m_audioDevice; }
}
