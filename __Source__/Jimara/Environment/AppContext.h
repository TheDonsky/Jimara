#pragma once
#include "../Graphics/GraphicsDevice.h"
#include "../Graphics/Data/GraphicsMesh.h"
#include "../Physics/PhysicsInstance.h"
#include "../Audio/AudioInstance.h"


namespace Jimara {
	class AppContext : public virtual Object {
	public:
		AppContext(Graphics::GraphicsDevice* device, Physics::PhysicsInstance* physics = nullptr, Audio::AudioDevice* audioDevice = nullptr);

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

		Graphics::ShaderCache* ShaderCache()const;

		Graphics::GraphicsMeshCache* GraphicsMeshCache()const;

		Physics::PhysicsInstance* PhysicsInstance()const;

		Audio::AudioDevice* AudioDevice()const;

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Graphics::ShaderCache> m_shaderCache;
		const Reference<Graphics::GraphicsMeshCache> m_graphicsMeshCache;
		const Reference<Physics::PhysicsInstance> m_physicsInstance;
		const Reference<Audio::AudioDevice> m_audioDevice;
	};
}
