#pragma once
#include "../Graphics/GraphicsDevice.h"
#include "../Graphics/Data/GraphicsMesh.h"


namespace Jimara {
	class AppContext : public virtual Object {
	public:
		AppContext(Graphics::GraphicsDevice* device);

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

		Graphics::GraphicsMeshCache* GraphicsMeshCache()const;

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Graphics::GraphicsMeshCache> m_graphicsMeshCache;
	};
}
